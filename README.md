# Mandelbrot Image Generator
A *"blazingly"* fast, single-file C++ program that generates an image of the Mandelbrot set. Includes customizable color passes, multi-threaded rendering, and a simple raw bitmap output.

![Preview 1](./examples/1mp-1.bmp)
![Preview 2](./examples/1mp-2.bmp)
![Preview 3](./examples/1mp-3.bmp)

## Building

Compile with GCC or Clang. It is best to use `O3` precision, which will vastly decrease rendering times. Recommended build command:

```bash
g++ -O3 ./src/main.cpp -o mandelbrot
```

## Usage

There are 5 variables you can control:
 - `X Position` (Default: 0.0)
 - `Y Position` (Default: 0.0)
 - `Scale` (Default: 2.0)
 - `Horizontal Resolution` (Default: 512)
 - `Vertical Resolution` (Default: 512)
 - `Chunk Count` (Default: 1)

Each do more-or-less what they say they do. X/Y Position is the origin point (in the complex plane) where the image will begin rendering. Scale is essentially the zoom level. Horizontal/Vertical Resolution define the resolution of the final output `.bmp`. 

Chunk count determines the number of threads to use while rendering. Increasing this number to the number of CPU threads you have will boost performance significantly (maximum: 255). On some CPUs you can also take advantage thread over-subscription, and set the chunk count to more than your number of CPU threads.

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

## AI Use Disclaimer
Neither generative, nor agentic models were used to a significant degree in the creation of this project. All code was written by me, and all debugging was done by hand.
