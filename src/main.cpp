// Generates an image of the mandelbrot set, a fractal based in the complex
// plane using the formula z=z^2+c. https://en.wikipedia.org/wiki/Mandelbrot_set
#include "../include/quadmath.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <stdfloat>
#include <stdlib.h>
#include <vector>

using namespace std;
using pfloat = __float128;

// Define argument defaults
static uint32_t g_width{512};
static uint32_t g_height{512};
static double g_xpos{0};
static double g_ypos{0};
static double g_scale{1};
static string g_output{"./image.bmp"};
static uint8_t g_chunksCount{2};
static uint8_t g_precision{40};

// Scaler and offset values
static double g_aspectRatio{};
static double g_horizontalScaler{};
static double g_horizontalOffset{};
static double g_verticalScaler{};
static double g_verticalOffset{};

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

// Complex double class
struct CompD {
  double real;
  double imag;

  CompD(double r, double i) {
    real = r;
    imag = i;
  }
};

// Precise complex double class
struct pCompD {
  pfloat real;
  pfloat imag;

  pCompD(pfloat r, pfloat i) {
    real = r;
    imag = i;
  }
};

// Multiply two complex doubles together
CompD multCompD(CompD a, CompD b) {
  double k1 = (a.real * b.real);
  double k2 = (a.imag * b.imag);
  double k3 = (a.real + a.imag) * (b.real + b.imag);
  return {(k1 - k2), (k3 - k1 - k2)};
}

// Add complex doubles
void addCompD(CompD &a, CompD b) {
  a = {(a.real + b.real), (a.imag + b.imag)};
  return;
}

void squareCompD(CompD &a) {
  a = {((a.real * a.real) - (a.imag * a.imag)), (2 * a.real * a.imag)};
  return;
}

// I DID NOT WRITE THIS ALGORITHM. This is a hyper-efficient approximation of
// taking the absolute value of a complex double.
double approxAbsCompD(CompD a) {
  double x = fabs(a.real);
  double y = fabs(a.imag);
  double max_val = (x > y) ? x : y;
  double min_val = (x > y) ? y : x;
  return 0.9604338f * max_val + 0.3978247f * min_val;
}

// Itterate once over the mandelbrot formula
void mandelbrotItter(CompD &a, CompD b) {
  double ar = a.real;
  double ai = a.imag;
  a = {((ar * ar) - (ai * ai) + b.real), ((2 * ar * ai) + b.imag)};
}
void mandelbrotItter(pCompD &a, pCompD b) {
  pfloat ar = a.real;
  pfloat ai = a.imag;
  a = {((ar * ar) - (ai * ai) + b.real), ((2 * ar * ai) + b.imag)};
}

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
void divergenceMap(double *m, double x, double y, bool accurate = false,
                   uint8_t max_itterations = 255) {
  // Reset map to base value. This should be 0x00 99% of the time.
  memset(m, 0x0, 256 * sizeof(double));
  // Define complex numbers z, and c in z=z^2+c
  CompD z{0, 0};
  CompD c{x, y};
  double a{0};
  double previous{0};

  // Calculate max divergence for each itteration
  mandelbrotItter(z, c);
  // The accurate boolean determines weather to approximate values, or calulate
  // them. Approximation can yield a performance boost of 30%-40%.
  if (accurate == false) {
    m[0] = approxAbsCompD(z);
  } else {
    m[0] = hypot(z.real, z.imag);
  }
  for (int i = 1; i <= max_itterations; i++) {
    previous = m[i - 1];
    // If z ever reaches 2, we can stop calculating, as it is provably not in
    // the set. This sacrafaces color-complexity for performance.
    if (previous >= 2.0f) {
      m[i] = 5.0f;
    } else {
      mandelbrotItter(z, c);
      if (accurate == false) {
        a = approxAbsCompD(z);
      } else {
        a = hypot(z.real, z.imag);
      }
      if (previous < a) {
        m[i] = a;
      } else {
        m[i] = previous;
      }
    }
  }

  // Go back over the map, and reset the values of anything we know is inside
  // of the set.
  if (m[max_itterations - 1] < 2.0f) {
    memset(m, 0x0, 256 * sizeof(double));
  }
  return;
}
void divergenceMap(pfloat *m, pfloat x, pfloat y,
                   uint8_t max_itterations = 255) {
  // Reset map to base value. This should be 0x00 99% of the time.
  memset(m, 0x0, 256 * sizeof(pfloat));
  // Define complex numbers z, and c in z=z^2+c
  pCompD z{0, 0};
  pCompD c{x, y};
  pfloat a{0};
  pfloat previous{0};

  // Calculate max divergence for each itteration
  mandelbrotItter(z, c);
  m[0] = hypotq(z.real, z.imag);
  for (int i = 1; i <= max_itterations; i++) {
    previous = m[i - 1];
    // If z ever reaches 2, we can stop calculating, as it is provably not in
    // the set. This sacrafaces color-complexity for performance.
    if (previous >= 2.0f) {
      m[i] = 5.0f;
    } else {
      mandelbrotItter(z, c);
      a = hypotq(z.real, z.imag);
      if (previous < a) {
        m[i] = a;
      } else {
        m[i] = previous;
      }
    }
  }

  // Go back over the map, and reset the values of anything we know is inside
  // of the set.
  if (m[max_itterations - 1] < 2.0f) {
    memset(m, 0x0, 256 * sizeof(pfloat));
  }
  return;
}

// Runs a color pass on a single pixel of an image. Takes in the image, a
// divergence map, x and y coordinates of the pixel, r g b a scales for output
// colors, and weather or not to quantize the pass.
void pass(Chunk &c, double map_val, int x, int y, int r, int g, int b,
          bool quantize = false) {
  int offset = x + (y * c.width);
  // Quantized version
  if (quantize) {
    c.red[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * r));
    c.green[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * g));
    c.blue[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * b));
  }
  // Unquantized/smooth version
  else {
    c.red[offset] ^= (static_cast<int>(map_val * r));
    c.green[offset] ^= (static_cast<int>(map_val * g));
    c.blue[offset] ^= (static_cast<int>(map_val * b));
  }
  return;
}
void pass(Chunk &c, pfloat map_val, int x, int y, int r, int g, int b,
          bool quantize = false) {
  int offset = x + (y * c.width);
  // Quantized version
  if (quantize) {
    c.red[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * r));
    c.green[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * g));
    c.blue[offset] ^= (static_cast<int>((static_cast<int>(map_val)) * b));
  }
  // Unquantized/smooth version
  else {
    c.red[offset] ^= (static_cast<int>(map_val * r));
    c.green[offset] ^= (static_cast<int>(map_val * g));
    c.blue[offset] ^= (static_cast<int>(map_val * b));
  }
  return;
}

// Function to process a single chunk with color passes
void processChunk(Chunk &c, int chunkIndex, uint8_t precision) {
  // Depending on the precision value, use precision floats (float128) or use
  // standard doubles
  if (precision < 64) {
    double map[256]{0};
    for (int i = 0; i < c.height; i++) {
      for (int j = 0; j < c.width; j++) {
        // Run divergence map calculation for the following passes
        divergenceMap(map,
                      (g_horizontalOffset +
                       (static_cast<double>(j) * g_horizontalScaler)),
                      (g_verticalOffset +
                       (static_cast<double>(i) + (c.height * (chunkIndex))) *
                           g_verticalScaler),
                      true, precision);
        // Run color passes
        //      pass(c, map[30] * 10.0f, j, i, 10, 10, 10, false);
        for (int k = 0; k < 40; k++) {
          pass(c, map[k] * 10.0f, j, i, 10, 10, 10, false);
        }
      }
    }
  } else {
    pfloat map[256]{0};
    for (int i = 0; i < c.height; i++) {
      for (int j = 0; j < c.width; j++) {
        // Run divergence map calculation for the following passes
        divergenceMap(map,
                      (g_horizontalOffset +
                       (static_cast<pfloat>(j) * g_horizontalScaler)),
                      (g_verticalOffset +
                       (static_cast<pfloat>(i) + (c.height * (chunkIndex))) *
                           g_verticalScaler),
                      precision);
        // Run color passes
        //      pass(c, map[30] * 10.0f, j, i, 10, 10, 10, false);
        for (int k = 0; k < 40; k++) {
          pass(c, map[k] * 10.0q, j, i, 10, 10, 10, false);
        }
      }
    }
  }
  return;
}

// Write the header of the .bmp file. Modified from the example files to
// support vairable image size
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

// Edit global variables based on program flags
void setGlobals(int argc, char **argv) {
  // Set global variables
  for (int i = 1; i < argc - 1; ++i) {
    if (argv[i][0 == '-']) {
      switch (argv[i][1]) {
      case 'x':
        g_xpos = stoi(argv[i + 1]);
        break;
      case 'y':
        g_ypos = stoi(argv[i + 1]);
        break;
      case 's':
        g_scale = stoi(argv[i + 1]);
        break;
      case 'w':
        g_width = stoi(argv[i + 1]);
        break;
      case 'h':
        g_height = stoi(argv[i + 1]);
        break;
      case 'r':
        g_width = stoi(argv[i + 1]);
        g_height = stoi(argv[i + 1]);
        break;
      case 't':
        g_chunksCount = stoi(argv[i + 1]);
        break;
      case 'p':
        g_precision = stoi(argv[i + 1]);
        break;
      }
    }
  }

  // Generate scaler constants for current chunk
  g_aspectRatio = {
      (static_cast<double>(g_height) / static_cast<double>(g_width))};
  g_horizontalScaler = {2.0f * (g_scale / static_cast<double>(g_width))};
  g_horizontalOffset = {g_xpos - g_scale};
  g_verticalScaler = {g_aspectRatio * 2.0f *
                      (g_scale / static_cast<double>(g_height))};
  g_verticalOffset = {g_aspectRatio *
                      (g_ypos * (static_cast<double>(g_chunksCount) /
                                 static_cast<double>(g_height)) -
                       g_scale)};
}

int main(int argc, char **argv) {
  // Start timer
  auto timeStart = chrono::high_resolution_clock::now();

  // Set globals
  setGlobals(argc, argv);
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
        async(launch::async, processChunk, ref(*cc.chunks[i]), i, g_precision));
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
       << "s (" << setprecision(4)
       << ((static_cast<double>(g_width * g_height) / 1000000.0f) /
           (duration.count() / 1000000.0f))
       << " megapixels/second)" << endl;

  // Clean up and exit
  fout.close();
  return 0;
}
