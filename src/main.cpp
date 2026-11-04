// Generates an image of the mandelbrot set, a fractal based in the complex
// plane using the formula z=z^2+c. https://en.wikipedia.org/wiki/Mandelbrot_set
#include <chrono>
#include <complex>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <vector>

using namespace std;

// Define argument defaults
static uint32_t g_width{512};
static uint32_t g_height{512};
static double g_xpos{-2};
static double g_ypos{-1.5};
static double g_scale{3};
static string g_output{"./image.bmp"};
static uint8_t g_chunksCount{2};

// Image struct. Stores g_width, g_height, arrays for 1-byte rgb values.
struct Chunk {
  uint32_t width;
  uint32_t height;
  uint8_t *red;
  uint8_t *green;
  uint8_t *blue;

  // Constructor, allocates 3 arrays of size w * h
  Chunk(uint32_t w, uint32_t h) {
    width = w;
    height = h;
    red = (uint8_t *)calloc(w * h, 1);
    green = (uint8_t *)calloc(w * h, 1);
    blue = (uint8_t *)calloc(w * h, 1);
  }

  // Destructor
  ~Chunk() {
    delete (red);
    delete (green);
    delete (blue);
  }
};

// A modified vector of chunks, stored for
struct ChunkContainer {
  uint8_t count;
  uint32_t width;
  uint32_t height;
  vector<Chunk *> chunks;

  ChunkContainer(uint32_t w, uint32_t h, uint8_t c) {
    count = c;
    width = w;
    height = h;
    for (int i = 0; i < c; i++) {
      chunks.push_back(new Chunk(w, h / c));
    }
  }

  ~ChunkContainer() {
    for (int i = 0; i < count; i++) {
      free(chunks[i]);
    }
  }
};

// Ouputs an Image to an ofsteam. Why the hell do bitmaps take values in bgr
// instead of rgb????
void outputChunk(ofstream &f, Chunk &c) {
  for (int i = 0; i < c.height; i++) {
    for (int j = 0; j < c.width; j++) {
      f << c.blue[j + (i * c.width)] << c.green[j + (i * c.width)]
        << c.red[j + (i * c.width)];
    }
  }
  return;
}

// Updated way of calculating divergence. It stores the maximum values of all
// itterations (up to max_itterations) in an array. This can then be indexed
// instead of calling the divergence function every time.
void divergenceMap(double *m, double x, double y, uint8_t max_itterations = 255,
                   uint8_t baseValue = 0x0) {
  // Reset map to base value. This should be 0x00 99% of the time.
  memset(m, baseValue, 256 * sizeof(double));
  // Define complex numbers z, and c in z=z^2+c
  complex<double> z(0, 0);
  complex<double> c(((g_xpos) + (x * (g_scale / g_width))),
                    ((g_ypos) + (y * (g_scale / g_height))));
  // Calculate max divergence for each itteration
  z = (z * z) + c;
  m[0] = abs(z);
  for (int i = 1; i <= max_itterations; i++) {
    z = (z * z) + c;
    if (m[i - 1] < abs(z)) {
      m[i] = abs(z);
    }
  }
  return;
}

// Legacy version of divergenceMap. Returns the value of z = z^2 + c for the
// complex number defined by c=x+yi after n itterations.
int divergence(double x, double y, int n) {
  double dx = static_cast<double>(x);
  double dy = static_cast<double>(y);
  // Define complex numbers z, and c in z=z^2+c, as well as max
  // which will store the maximum divergence
  complex<double> z(0, 0);
  complex<double> c(((g_xpos) + (dx * (g_scale / g_width))),
                    ((g_ypos) + (dy * (g_scale / g_height))));
  int max{0};

  // Calculate the mandelbrot formula
  for (int i = 0; i < n; i++) {
    z = (z * z) + c;
    if (abs(z) > max) {
      max = abs(z);
    }
  }

  // return the maximum divergence.
  return (max / n) % 255;
}

// Runs a color pass on a single pixel of an image. Takes in the image, a
// divergence map, x and y coordinates of the pixel, r g b a scales for output
// colors, and weather or not to quantize the pass.
void pass(Chunk &c, double *m, int x, int y, int r, int g, int b,
          uint8_t itterations = 1, double alpha = 1.0f, bool quantize = true) {
  // Attempt at adjusting the alpha channel to the number of itterations.
  double adjustedAlpha = (alpha / (itterations * itterations));
  // Quantized version
  if (quantize) {
    c.red[x + (y * c.width)] ^= (static_cast<int>(
        (static_cast<int>(m[itterations]) / itterations) * r * adjustedAlpha));
    c.green[x + (y * c.width)] ^= (static_cast<int>(
        (static_cast<int>(m[itterations]) / itterations) * g * adjustedAlpha));
    c.blue[x + (y * c.width)] ^= (static_cast<int>(
        (static_cast<int>(m[itterations]) / itterations) * b * adjustedAlpha));
  }
  // Unquantized/smooth version
  else {
    c.red[x + (y * c.width)] ^=
        (static_cast<int>((m[itterations] / itterations) * r * adjustedAlpha));
    c.green[x + (y * c.width)] ^=
        (static_cast<int>((m[itterations] / itterations) * g * adjustedAlpha));
    c.blue[x + (y * c.width)] ^=
        (static_cast<int>((m[itterations] / itterations) * b * adjustedAlpha));
  }
  return;
}

// Legacy version of the pass function. Runs a color pass on the image. For each
// color channel, run a divergence calculation with a n defined by the r g b
// values. You can also set the alpha channel. Performans a bitwise xor with the
// previous pass and the new divergence values.
void pass(Chunk &c, int x, int y, int r, int g, int b, float alpha = 1.0f) {
  c.red[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, r)) * alpha);
  c.green[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, g)) * alpha);
  c.blue[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, b)) * alpha);
}

void processChunk(Chunk &c, int chunkIndex) {
  double *map = (double *)calloc(256, sizeof(double));
  for (int i = 0; i < c.height; i++) {
    for (int j = 0; j < c.width; j++) {
      // Run a divergence map calculation that will be used in subsequent
      // passes. Remember to set max_itterations to the largest number of
      // itterations used in the following passes.
      divergenceMap(map, j, (i + c.height * chunkIndex), 40, 0x00);
      // Run multiple passes. The numbers define the colors, and are just
      // vibes.
      pass(c, map, j, i, 6, 4, 1, 40);
      pass(c, map, j, i, 5, 3, 2, 30);
      pass(c, map, j, i, 1, 2, 3, 7, 0.5f, false);
      pass(c, map, j, i, 2, 3, 5, 2, 8.0f, false);
    }
  }
  return;
}

// Write the header of the .bmp file. Modified from the example files to support
// vairable image size
void writeHeader(ofstream &f, uint32_t w, uint32_t h) {
  char format_sig[2] = {'B', 'M'};
  uint32_t file_size = 54 + (w * h * 3);
  uint32_t reserved_bytes = 0;
  uint32_t offset = 54;
  uint32_t header_size = 40;
  uint32_t g_width = w;
  uint32_t g_height = h;
  uint16_t num_color_planes = 1;
  uint16_t color_depth = 24;
  uint32_t compression = 0;
  uint32_t raw_data_size = 0;
  uint32_t h_res = 3780;
  uint32_t v_res = 3780;
  uint32_t c_table = 0;
  uint32_t i_colors = 0;
  f.write(format_sig, 2);
  f.write((char *)&file_size, sizeof(uint32_t));
  f.write((char *)&reserved_bytes, sizeof(uint32_t));
  f.write((char *)&offset, sizeof(uint32_t));
  f.write((char *)&header_size, sizeof(uint32_t));
  f.write((char *)&g_width, sizeof(int32_t));
  f.write((char *)&g_height, sizeof(int32_t));
  f.write((char *)&num_color_planes, sizeof(uint16_t));
  f.write((char *)&color_depth, sizeof(uint16_t));
  f.write((char *)&compression, sizeof(uint32_t));
  f.write((char *)&raw_data_size, sizeof(uint32_t));
  f.write((char *)&h_res, sizeof(uint32_t));
  f.write((char *)&v_res, sizeof(uint32_t));
  f.write((char *)&c_table, sizeof(uint32_t));
  f.write((char *)&i_colors, sizeof(uint32_t));
  return;
}

int main(int argc, char **argv) {
  // Start timer
  auto timeStart = chrono::high_resolution_clock::now();

  switch (argc) {
  case 1:
    break;
  case 2:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    break;
  case 4:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    g_scale = stod(argv[3]);
    break;
  case 6:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    g_scale = stod(argv[3]);
    g_width = stoi(argv[4]);
    g_height = stoi(argv[5]);
    break;
  case 7:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    g_scale = stod(argv[3]);
    g_width = stoi(argv[4]);
    g_height = stoi(argv[5]);
    g_chunksCount = stoi(argv[6]);
    break;
  default:
    cout << "Invalid number of arguments." << endl;
    return 1;
    break;
  }
  cout << "X Position: " << g_xpos << endl
       << "Y Position: " << g_ypos << endl
       << "Scale: " << g_scale << endl
       << "Horizontal Resolution: " << g_width << endl
       << "Vertical Resolution: " << g_height << endl
       << "Chunk Count: " << +g_chunksCount << endl;

  // Open file
  ofstream fout(g_output, ios::binary);

  // Generate file header
  writeHeader(fout, g_width, g_height);

  // Create a new chunk container
  ChunkContainer cc(g_width, g_height, g_chunksCount);

  // Create a thread for each chunk
  vector<future<void>> workers;
  for (int i = 0; i < cc.count; i++) {
    workers.emplace_back(
        async(launch::async, processChunk, ref(*cc.chunks[i]), i));
  }
  // Wait for rendering to be complete
  for (auto &fut : workers) {
    fut.get();
  }

  // Export chunks to image file
  for (int c = 0; c < cc.count; c++) {
    outputChunk(fout, *cc.chunks[c]);
  }
  cout << endl;

  // Print time to complete
  auto timeStop = chrono::high_resolution_clock::now();
  auto duration =
      chrono::duration_cast<chrono::microseconds>(timeStop - timeStart);
  cout << "Finished in " << setprecision(4) << (duration.count() / 1000000.0f)
       << "s" << endl;

  // Clean up and exit
  fout.close();
  return 0;
}
