#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "image_io.h"

void sobel_serial(unsigned char *input, unsigned char *output,
                  int width, int height) {

    // Sobel kernels
    int Gx[3][3] = {
        {-1,  0,  1},
        {-2,  0,  2},
        {-1,  0,  1}
    };
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    // Process every pixel (skip border pixels)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {

            int gx = 0, gy = 0;

            // Apply 3x3 kernel
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = input[(y + ky) * width + (x + kx)];
                    gx += Gx[ky + 1][kx + 1] * pixel;
                    gy += Gy[ky + 1][kx + 1] * pixel;
                }
            }

            // Calculate gradient magnitude
            int mag = (int)sqrt((double)(gx * gx + gy * gy));
            output[y * width + x] = (mag > 255) ? 255 : mag;
        }
    }
}

int main() {
    unsigned char *input, *output;
    int width, height;

    printf("=== Serial Edge Detection ===\n\n");

    // Load image
    if (read_pgm("input.pgm", &input, &width, &height) != 0)
        return 1;

    output = (unsigned char *)calloc(width * height, 1);

    // Run and time it
    clock_t start = clock();
    sobel_serial(input, output, width, height);
    clock_t end = clock();

    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n--- Results ---\n");
    printf("Time:       %.4f seconds\n", time_taken);
    printf("Pixels:     %d\n", width * height);

    // Save output
    write_pgm("output_serial.pgm", output, width, height);

    free(input);
    free(output);
    return 0;
}