# Mandelbrot Image Generator
A single-file C++ program that generates an image of the Mandelbrot set. Includes customizable color passes, multi-threaded rendering, and a simple raw bitmap output.

![Preview 1](./examples/1mp-1.bmp)
![Preview 2](./examples/1mp-2.bmp)

## Building

Compile with gcc or clang. It is best to use `O3` precision, which will vastly decrease rendering times. Recommended build command:

```bash
g++ -O3 ./src/main.cpp -o mandelbrot
```

## Usage

There are 5 variables you can control:
 - `X Position` (Default: -2)
 - `Y Position` (Default: -1.5)
 - `Scale` (Default: 3)
 - `Horizontal Resolution` (Default: 512)
 - `Vertical Resolution` (Default: 512)
 - `Chunk Count` (Default: 1)

Each do more-or-less what they say they do. X/Y Position is the origin point (in the complex plane) where the image will begin rendering. Scale is essentially the zoom level. Horizontal/Vertical Resolution define the resolution of the final output `.bmp`. Chunk count determines the number of threads to use while rendering. Increasing this number to the number of CPU threads you have will boost performance significantly (maximum: 255).

The program works with the following argument formats:

```bash
./mandelbrot
```
```bash
./mandelbrot [xpos] [ypos]
```
```bash
./mandelbrot [xpos] [ypos] [scale]
```
```bash
./mandelbrot [xpos] [ypos] [scale] [h_res] [v_res]
```
```bash
./mandelbrot [xpos] [ypos] [scale] [h_res] [v_res] [chunk_count]
```

### Color customization

In the future I plan on changing this as its currently somewhat complicated.
