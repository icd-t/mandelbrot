// Peter Janecek
// jst568@vols.utk.edu
// Generates an image of the mandelbrot set, a fractal based in the complex
// plane using the formula z=z^2+c. https://en.wikipedia.org/wiki/Mandelbrot_set
#include <complex>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdlib.h>

using namespace std;

// Define width/height
static uint32_t width{512};
static uint32_t height{512};

// Image struct. Stores width, height, arrays for 1-byte rgb values.
struct Image {
  uint32_t width;
  uint32_t height;
  uint8_t *red;
  uint8_t *green;
  uint8_t *blue;

  // Constructor, allocates 3 arrays of size w * h
  Image(uint32_t w, uint32_t h) {
    width = w;
    height = h;
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
  for (int i = 0; i < image.height; i++) {
    for (int j = 0; j < image.width; j++) {
      f << image.blue[j + (i * image.width)]
        << image.green[j + (i * image.width)]
        << image.red[j + (i * image.width)];
    }
  }
  return;
}

// Returns the value of z = z^2 + c for the complex number defined by c=x+yi
// after n itterations.
int divergence(double x, double y, int n) {
  double dx = static_cast<double>(x);
  double dy = static_cast<double>(y);
  double scale = 255.0f / 1.5f;
  // Define complex numbers z, and c in z=z^2+c, as well as max
  // which will store the maximum divergence
  complex<double> z(0, 0);
  complex<double> c(((dx - (scale * 2)) / scale),
                    ((dy - (scale * 1.5f)) / scale));
  int max{0};

  // Calculate the mandelbrot formula
  for (int i = 0; i < n; i++) {
    z = (z * z) + c;
    if (abs(z) > max) {
      max = abs(z);
    }
  }

  // return the maximum divergence.
  return max;
}

// Runs a color pass on the image. For each color channel, run a divergence
// calculation with a n defined by the r g b values. You can also set the alpha
// channel. Performans a bitwise xor with the previous pass and the new
// divergence values.
void pass(Image &i, int x, int y, int r, int g, int b, float alpha = 1.0f) {
  i.red[x + (y * width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, r)) * alpha);
  i.green[x + (y * width)] ^=
      static_cast<int>(static_cast<double>(divergence(x, y, g)) * alpha);
  i.blue[x + (y * width)] ^=
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
  uint32_t width = w;
  uint32_t height = h;
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
  f.write((char *)&width, sizeof(int32_t));
  f.write((char *)&height, sizeof(int32_t));
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
  // Open file
  ofstream fout("output.bmp", ios::binary);

  // Generate file header
  writeHeader(fout, width, height);

  // Create image object
  Image img = Image(width, height);

  // Loop over all pixels
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      // Run multiple passes. The numbers define the colors, and are just vibes.
      // They don't look that good tbh.
      pass(img, j, i, 1, 1, 1);
      pass(img, j, i, 3, 3, 3);
      pass(img, j, i, 1, 2, 3);
      pass(img, j, i, 13, 17, 20);
      pass(img, j, i, 100, 75, 50);
      pass(img, j, i, i % 5, j % 5, (i * j) % 5);
      pass(img, j, i, i % 10, j % 10, (i * j) % 10, 0.0001);
    }
  }

  // Output img to the file
  outputImage(fout, img);

  // Clean up and exit
  fout.close();
  return 0;
}
