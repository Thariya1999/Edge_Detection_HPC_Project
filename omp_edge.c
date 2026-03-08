#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <omp.h>
#include "image_io.h"

// ─── Sobel Serial ─────────────────────────────────────────
void sobel_serial(unsigned char *input, unsigned char *output,
                  int width, int height) {
    int Gx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    int Gy[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};

    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++) {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ky++)
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = input[(y+ky)*width+(x+kx)];
                    gx += Gx[ky+1][kx+1] * pixel;
                    gy += Gy[ky+1][kx+1] * pixel;
                }
            int mag = (int)sqrt((double)(gx*gx + gy*gy));
            output[y*width+x] = (mag > 255) ? 255 : mag;
        }
}

// ─── Sobel OpenMP ─────────────────────────────────────────
void sobel_omp(unsigned char *input, unsigned char *output,
               int width, int height, int num_threads) {
    int Gx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    int Gy[3][3] = {{-1,-2,-1},{0,0,0},{1,2,1}};

    omp_set_num_threads(num_threads);
    #pragma omp parallel
    {
        #pragma omp for schedule(static) nowait
        for (int y = 1; y < height - 1; y++)
            for (int x = 1; x < width - 1; x++) {
                int gx = 0, gy = 0;
                for (int ky = -1; ky <= 1; ky++)
                    for (int kx = -1; kx <= 1; kx++) {
                        int pixel = input[(y+ky)*width+(x+kx)];
                        gx += Gx[ky+1][kx+1] * pixel;
                        gy += Gy[ky+1][kx+1] * pixel;
                    }
                int mag = (int)sqrt((double)(gx*gx + gy*gy));
                output[y*width+x] = (mag > 255) ? 255 : mag;
            }
    }
}

// ─── Warmup Thread Pool ───────────────────────────────────
void warmup(int num_threads) {
    omp_set_num_threads(num_threads);
    #pragma omp parallel
    { }
}

// ─── Summary Storage ──────────────────────────────────────
#define MAX_IMAGES 200

typedef struct {
    char   name[256];    // FIX: match base[] size
    int    width, height;
    double serial_time;
    double omp1_time;
    double omp2_time;
    double omp4_time;
} ImageResult;

ImageResult results[MAX_IMAGES];
int result_count = 0;

// ─── Check if Path is Directory ───────────────────────────
int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0)
        return S_ISDIR(st.st_mode);
    return 0;
}

// ─── Check if File is PGM ─────────────────────────────────
int is_pgm(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".pgm") == 0 || strcmp(dot, ".PGM") == 0);
}

// ─── Process One PGM Image ────────────────────────────────
void process_image(const char *pgm_path) {

    unsigned char *input, *output;
    int width, height;

    // Extract base name
    char base[256];
    const char *slash = strrchr(pgm_path, '/');
    const char *fname = slash ? slash + 1 : pgm_path;
    strncpy(base, fname, sizeof(base) - 1);
    base[sizeof(base) - 1] = '\0';
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';

    printf("\n┌─────────────────────────────────────────┐\n");
    printf("│ IMAGE: %-33s│\n", base);
    printf("└─────────────────────────────────────────┘\n");

    if (read_pgm(pgm_path, &input, &width, &height) != 0) {
        printf("  ERROR: Could not open %s\n", pgm_path);
        printf("  Skipping...\n");
        return;
    }

    output = (unsigned char *)calloc(width * height, 1);
    int repeat = 3;

    // ── Serial ──
    double serial_best = 1e9;
    for (int r = 0; r < repeat; r++) {
        memset(output, 0, width * height);
        double s = omp_get_wtime();
        sobel_serial(input, output, width, height);
        double e = omp_get_wtime();
        if ((e - s) < serial_best) serial_best = e - s;
    }

    // Save serial output
    char serial_out[512];
    sprintf(serial_out, "outputs/%s_serial.pgm", base);
    write_pgm(serial_out, output, width, height);

    printf("\n  %-10s %-12s %-10s %-10s\n",
           "Mode", "Time(sec)", "Speedup", "Efficiency");
    printf("  ──────────────────────────────────────────\n");
    printf("  %-10s %-12.4f %-10.2f %-10s\n",
           "Serial", serial_best, 1.0, "100.00%");

    // ── OpenMP 1, 2, 4 threads ──
    int    thread_counts[] = {1, 2, 4};
    double omp_times[3];

    for (int t = 0; t < 3; t++) {
        int threads = thread_counts[t];
        double best = 1e9;

        for (int r = 0; r < repeat; r++) {
            memset(output, 0, width * height);
            double s = omp_get_wtime();
            sobel_omp(input, output, width, height, threads);
            double e = omp_get_wtime();
            if ((e - s) < best) best = e - s;
        }
        omp_times[t] = best;

        double speedup    = serial_best / best;
        double efficiency = (speedup / threads) * 100.0;

        char label[20];
        sprintf(label, "OMP-%d", threads);

        char eff_str[20];
        sprintf(eff_str, "%.2f%%", efficiency);

        printf("  %-10s %-12.4f %-10.2f %-10s\n",
               label, best, speedup, eff_str);
    }
    printf("  ──────────────────────────────────────────\n");

    // Save OMP-4 output
    char omp_out[512];
    sprintf(omp_out, "outputs/%s_omp4.pgm", base);
    memset(output, 0, width * height);
    sobel_omp(input, output, width, height, 4);
    write_pgm(omp_out, output, width, height);

    // Store result for summary
    if (result_count < MAX_IMAGES) {
        snprintf(results[result_count].name, sizeof(results[result_count].name), "%s", base);
        results[result_count].width         = width;
        results[result_count].height        = height;
        results[result_count].serial_time   = serial_best;
        results[result_count].omp1_time     = omp_times[0];
        results[result_count].omp2_time     = omp_times[1];
        results[result_count].omp4_time     = omp_times[2];
        result_count++;
    }

    free(input);
    free(output);
}

// ─── Process Entire Directory ─────────────────────────────
void process_directory(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        printf("ERROR: Cannot open directory: %s\n", dirpath);
        return;
    }

    // Count PGM files first
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL)
        if (is_pgm(entry->d_name)) count++;
    rewinddir(dir);

    if (count == 0) {
        printf("ERROR: No .pgm files found in %s\n", dirpath);
        printf("Convert your images first:\n");
        printf("  ./convert_bulk.sh %s\n", dirpath);
        closedir(dir);
        return;
    }

    printf("Found %d PGM image(s) in folder: %s\n", count, dirpath);

    while ((entry = readdir(dir)) != NULL) {
        if (!is_pgm(entry->d_name)) continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 dirpath, entry->d_name);
        process_image(full_path);
    }

    closedir(dir);
}

// ─── Print Final Summary ──────────────────────────────────
void print_summary() {
    if (result_count == 0) return;

    printf("\n");
    printf("╔═════════════════════════════════════════════════════════════════════╗\n");
    printf("║                        FINAL SUMMARY                                ║\n");
    printf("╠══════════════╦══════════════╦══════════╦══════════╦══════════╦══════╣\n");
    printf("║ %-12s ║ %-12s ║ %-8s ║ %-8s ║ %-8s ║ %-4s ║\n",
           "Image", "Size", "Serial", "OMP-1", "OMP-2", "OMP-4");
    printf("╠══════════════╬══════════════╬══════════╬══════════╬══════════╬══════╣\n");

    double total_s2 = 0, total_s4 = 0;

    for (int i = 0; i < result_count; i++) {
        char size[20];
        sprintf(size, "%dx%d", results[i].width, results[i].height);

        // Truncate long names for table display
        char shortname[17];
        strncpy(shortname, results[i].name, 16);  // FIX
        shortname[16] = '\0';                      // FIX

        printf("║ %-12s ║ %-12s ║ %-8.4f ║ %-8.4f ║ %-8.4f ║%-6.4f║\n",
               shortname, size,
               results[i].serial_time,
               results[i].omp1_time,
               results[i].omp2_time,
               results[i].omp4_time);

        total_s2 += results[i].serial_time / results[i].omp2_time;
        total_s4 += results[i].serial_time / results[i].omp4_time;
    }

    printf("╠══════════════╩══════════════╩══════════╩══════════╩══════════╩══════╣\n");
    printf("║  Total Images : %-3d                                                ║\n",
           result_count);
    printf("║  Avg Speedup OMP-2 : %-5.2fx   Avg Speedup OMP-4 : %-5.2fx          ║\n",
           total_s2 / result_count,
           total_s4 / result_count);
    printf("╚═════════════════════════════════════════════════════════════════════╝\n");
    printf("\nOutputs saved in: outputs/\n");
}

// ─── Main ─────────────────────────────────────────────────
int main(int argc, char *argv[]) {

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   Serial vs OpenMP Edge Detection Tool   ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("CPU Cores available: %d\n\n", omp_get_max_threads());

    warmup(omp_get_max_threads());
    printf("Thread pool ready!\n\n");

    int ret = system("mkdir -p outputs");
    (void)ret;

    // ── Show usage if no arguments ──
    if (argc < 2) {
        printf("USAGE:\n\n");
        printf("  Single image:\n");
        printf("    ./omp_edge images/photo.pgm\n\n");
        printf("  Multiple images:\n");
        printf("    ./omp_edge images/p1.pgm images/p2.pgm images/p3.pgm\n\n");
        printf("  Entire folder:\n");
        printf("    ./omp_edge images/\n\n");
        printf("  Mix folder + single images:\n");
        printf("    ./omp_edge images/ extra/special.pgm\n\n");
        printf("HOW TO CONVERT PHOTOS:\n");
        printf("  Single:  ./convert_photo.sh photo.jpg\n");
        printf("  Folder:  ./convert_bulk.sh /mnt/c/Users/ACER/Downloads/folder/\n");
        return 0;
    }

    // ── Process each argument ──
    printf("Input arguments: %d\n", argc - 1);

    for (int i = 1; i < argc; i++) {
        if (is_directory(argv[i])) {
            printf("\nFOLDER: %s\n", argv[i]);
            process_directory(argv[i]);
        } else if (is_pgm(argv[i])) {
            process_image(argv[i]);
        } else {
            printf("\nSKIPPED: %s\n", argv[i]);
            printf("  Reason: not a .pgm file or folder\n");
            printf("  Convert first: ./convert_photo.sh %s\n", argv[i]);
        }
    }

    // ── Final summary ──
    print_summary();

    return 0;
}