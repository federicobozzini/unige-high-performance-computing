#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// USAGE: mandelbrot_seq_2d <rows> <cols>
// OUTPUT: PERFORMANCE IN TIME SPENT

#define TRIALS 2

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
    int rows, cols, size, i, j, k, iteration, max_iteration, **grid;
    double x0, y0, x, y, xmin, xmax, ymin, ymax, xtemp, ttot, tstart, tend, tminseq, tminpar;
    char filename[] = "mandelbrot_seq.dat";

    if (argc < 3)
    {
        printf("Usage: mandelbrot_seq cols rows\n");
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
    xmin = -2.5;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    grid = (int **)malloc(cols * sizeof(int *));
    for (i = 0; i < cols; i++)
    {
        grid[i] = (int *)malloc(rows * sizeof(int));
    }

    for (k = 0; k < TRIALS; k++)
    {
        tminseq = 10e10;

        tstart = get_time();

        for (i = 0; i < cols; i++)
        {
            for (j = 0; j < rows; j++)
            {
                x0 = (double)j / (rows - 1) * (xmax - xmin) + xmin;
                y0 = (double)i / (cols - 1) * (ymax - ymin) + ymin;
                x = 0;
                y = 0;
                iteration = 0;
                while (x * x + y * y < 2 * 2 && iteration < max_iteration)
                {
                    xtemp = x * x - y * y + x0;
                    y = 2 * x * y + y0;
                    x = xtemp;
                    iteration++;
                }
                grid[i][j] = iteration;
            }
        }

        tend = get_time();

        ttot = tend - tstart;

        if (ttot < tminseq)
            tminseq = ttot;
    }

    printf("%.2lf\n", tminseq / 10e6);

    fp = fopen(filename, "w");

    for (i = 0; i < cols; i++)
    {
        for (j = 0; j < rows; j++)
        {
            fprintf(fp, "%i ", grid[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    for (i = 0; i < cols; i++)
    {
        free(grid[i]);
    }
    free(grid);

    return 0;
}
