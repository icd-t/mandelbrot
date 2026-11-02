// Generates an image of the mandelbrot set, a fractal based in the complex
// plane using the formula z=z^2+c. https://en.wikipedia.org/wiki/Mandelbrot_set
#include <chrono>
#include <complex>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <stdlib.h>

using namespace std;

// Define argument defaults
static uint32_t g_width{512};
static uint32_t g_height{512};
static double g_xpos{0};
static double g_ypos{0};
static double g_scale{1.5};
static string g_output{"./image.bmp"};

// Image struct. Stores g_width, g_height, arrays for 1-byte rgb values.
struct Image {
  uint32_t g_width;
  uint32_t g_height;
  uint8_t *red;
  uint8_t *green;
  uint8_t *blue;

  // Constructor, allocates 3 arrays of size w * h
  Image(uint32_t w, uint32_t h) {
    g_width = w;
    g_height = h;
    red = (uint8_t *)calloc(w * h, 1);
    green = (uint8_t *)calloc(w * h, 1);
    blue = (uint8_t *)calloc(w * h, 1);
  }

  // Destructor
  ~Image() {
    free(red);
    free(green);
    free(blue);
  }
};

// Ouputs an Image to an ofsteam. Why the hell do bitmaps take values in bgr
// instead of rgb????
void outputImage(ofstream &f, Image &image) {
  for (int i = 0; i < image.g_height; i++) {
    for (int j = 0; j < image.g_width; j++) {
      f << image.blue[j + (i * image.g_width)]
        << image.green[j + (i * image.g_width)]
        << image.red[j + (i * image.g_width)];
    }
  }
  return;
}

// Returns the value of z = z^2 + c for the complex number defined by c=x+yi
// after n itterations.
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

// Runs a color pass on the image. For each color channel, run a divergence
// calculation with a n defined by the r g b values. You can also set the alpha
// channel. Performans a bitwise xor with the previous pass and the new
// divergence values.
void pass(Image &i, int x, int y, int r, int g, int b, float alpha = 1.0f) {
  i.red[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, r)) * alpha);
  i.green[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, g)) * alpha);
  i.blue[x + (y * g_width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, b)) * alpha);
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
    cout << "No arguments supplied, using defauls." << endl;
    break;
  case 2:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    cout << "X Position: " << g_xpos << endl
         << "Y Position: " << g_ypos << endl;
    break;
  case 4:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    g_scale = stod(argv[3]);
    cout << "X Position: " << g_xpos << endl
         << "Y Position: " << g_ypos << endl
         << "Scale: " << g_scale << endl;
    break;
  case 6:
    g_xpos = stod(argv[1]);
    g_ypos = stod(argv[2]);
    g_scale = stod(argv[3]);
    g_width = stoi(argv[4]);
    g_height = stoi(argv[5]);
    cout << "X Position: " << g_xpos << endl
         << "Y Position: " << g_ypos << endl
         << "Scale: " << g_scale << endl
         << "Horizontal Resolution: " << g_width << endl
         << "Vertical Resolution: " << g_height << endl;
    break;
  default:
    cout << "Invalid number of arguments." << endl;
    return 1;
    break;
  }

  // Open file
  ofstream fout(g_output, ios::binary);

  // Generate file header
  writeHeader(fout, g_width, g_height);

  // Create image object
  Image img = Image(g_width, g_height);

  // Loop over all pixels
  cout << endl << endl;
  double progress{0};
  for (int i = 0; i < g_height; i++) {
    for (int j = 0; j < g_width; j++) {
      // Run multiple passes. The numbers define the colors, and are just vibes.
      pass(img, j, i, 30, 25, 20, 1);
      pass(img, j, i, i % 5, j % 5, (i * j) % 5, 10);
      pass(img, j, i, i % 10, 1, (j) % 10, 1);
    }
    // Calculate and print progress to terminal
    progress = (static_cast<double>(i) / static_cast<double>(g_width)) * 100.0f;
    cout << "\u001b[FProgress: (" << fixed << setprecision(2) << progress
         << "%)" << endl;
  }
  cout << endl;

  // Output img to the file
  cout << "Outputting to " << g_output << endl;
  outputImage(fout, img);

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
