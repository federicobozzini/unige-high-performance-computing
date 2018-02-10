#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

// USAGE: mandelbrot_omp <rows> <cols> <task_size> <x0> <y0> <dx> <dy>
// OUTPUT: PERFORMANCE IN TIME SPENT

#define TRIALS 2
#define OMP_CHUNK_SIZE 100

double get_time()
{
    struct timespec tt;
    clock_gettime(CLOCK_REALTIME, &tt);
    double t = (double)tt.tv_sec * 1.0e9 + (double)tt.tv_nsec;
    return t;
}

int main(int argc, char **argv)
{
    FILE *fp;
    int rows, cols, size, i, j, k, max_iteration, *grid;
    double ttot, tstart, tend, tmin;
    char filename[] = "results/mandelbrot_omp.dat";

    if (argc < 3)
    {
        printf("Usage: mandelbrot_omp cols rows\n");
        return 1;
    }
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);
    size = rows * cols;

    if (rows < 2 || cols < 2)
    {
        printf("Error: cols and rows must be > 2\n");
        return 1;
    }

    max_iteration = 100;
    double xmin = argc > 4 ? atof(argv[4]) : -2.5;
    double ymin = argc > 5 ? atof(argv[5]) : -1;
    double xmax = argc > 6 ? xmin + atof(argv[6]) : 1;
    double ymax = argc > 7 ? ymin + atof(argv[7]) : 1;

    if (xmin >= xmax || ymin >= ymax)
    {
        printf("Usage: mandelbrot_mpi cols rows x0=-2.5 y0=-1 dx=-1 dy=1\n");
        return 1;
    }

    grid = (int *)malloc(size * sizeof(int));

    for (k = 0; k < TRIALS; k++)
    {
        tmin = 10e10;

        tstart = get_time();

#pragma omp parallel for schedule(static, OMP_CHUNK_SIZE)
        for (i = 0; i < size; i++)
        {
            int px = i % rows;
            int py = i / rows;
            double x0 = (double)px / (rows - 1) * (xmax - xmin) + xmin;
            double y0 = (double)py / (cols - 1) * (ymax - ymin) + ymin;
            double x = 0;
            double y = 0;
            int iteration = 0;
            while (x * x + y * y < 2 * 2 && iteration < max_iteration)
            {
                double xtemp = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = xtemp;
                iteration++;
            }
            grid[i] = iteration;
        }

        tend = get_time();

        ttot = tend - tstart;

        if (ttot < tmin)
            tmin = ttot;
    }

    printf("%.2lf\n", tmin / 10e6);

    fp = fopen(filename, "w");

    for (i = 0; i < cols; i++)
    {
        for (j = 0; j < rows; j++)
        {
            fprintf(fp, "%i ", grid[rows * i + j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    free(grid);

    return 0;
}