#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include <stdio.h>
#include <stdlib.h>

int read_pgm(const char *filename, unsigned char **data,
             int *width, int *height) {

    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("ERROR: Cannot open input.pgm");
        return -1;
    }

    char magic[3];
    // FIX 1: store fscanf return value
    if (fscanf(f, "%2s\n", magic) != 1) {
        fclose(f); return -1;
    }

    // Skip comment lines
    char c;
    while ((c = fgetc(f)) == '#') {
        while (fgetc(f) != '\n');
    }
    ungetc(c, f);

    // FIX 2: store fscanf return value
    if (fscanf(f, "%d %d\n%*d\n", width, height) != 2) {
        fclose(f); return -1;
    }

    *data = (unsigned char *)malloc((*width) * (*height));

    // FIX 3: store fread return value
    size_t read_count = fread(*data, 1, (*width) * (*height), f);
    if ((int)read_count != (*width) * (*height)) {
        printf("WARNING: Expected %d bytes, got %zu\n",
               (*width) * (*height), read_count);
    }

    fclose(f);
    printf("Image loaded: %d x %d pixels\n", *width, *height);
    return 0;
}

void write_pgm(const char *filename, unsigned char *data,
               int width, int height) {
    FILE *f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n255\n", width, height);
    fwrite(data, 1, width * height, f);
    fclose(f);
    printf("Output saved: %s\n", filename);
}

#endif