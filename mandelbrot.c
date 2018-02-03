#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

// USAGE: mandelbrot <m> <n>
// OUTPUT: PERFORMANCE IN TIME SPENT

#define SEQ_TRIALS 3
#define PAR_TRIALS 20
#define OMP_CHUNK_SIZE 100
#define MAXTIME 5000000000 /* 5 seconds */

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
    int m, n, size, i, j, k, iteration, max_iteration, **grid;
    double x0, y0, x, y, xmin, xmax, ymin, ymax, xtemp, ttot, tstart, tend, tminseq, tminpar;
    char filename[] = "mandelbrot.dat";

    if (argc < 3)
    {
        printf("Usage: mandelbrot n m\n");
        return 1;
    }
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    size = m * n;

    if (m < 2 || n < 2)
    {
        printf("Error: n and m must be > 2\n");
        return 1;
    }

    max_iteration = 100;
    xmin = -2.5;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    grid = (int **)malloc(n * sizeof(int *));
    for (i = 0; i < n; i++)
    {
        grid[i] = (int *)malloc(m * sizeof(int));
    }

    // SEQUENTIAL
    for (k = 0; k < SEQ_TRIALS; k++)
    {
        tminseq = 10e10;

        tstart = get_time();

        for (i = 0; i < n; i++)
        {
            for (j = 0; j < m; j++)
            {
                x0 = (double)j / (m - 1) * (xmax - xmin) + xmin;
                y0 = (double)i / (n - 1) * (ymax - ymin) + ymin;
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
        //printf("%i, time=%.2lf ms.\n", k, ttot / 10e6);

        if (ttot < tminseq)
            tminseq = ttot;
    }

    printf("sequential time=%.2lf ms.\n", tminseq / 10e6);

    //PARALLEL
    for (k = 0; k < PAR_TRIALS; k++)
    {
        tminpar = 10e10;

        tstart = get_time();

#pragma omp parallel for private(i, j, x0, y0, x, y, iteration, xtemp) schedule(static, OMP_CHUNK_SIZE) collapse(2) default(shared)
        for (i = 0; i < n; i++)
        {
            for (j = 0; j < m; j++)
            {
                x0 = (double)j / (m - 1) * (xmax - xmin) + xmin;
                y0 = (double)i / (n - 1) * (ymax - ymin) + ymin;
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
        //printf("%i, time=%.2lf ms.\n", k, ttot / 10e6);

        if (ttot < tminpar)
            tminpar = ttot;
    }

    printf("parallel time=%.2lf ms.\n", tminpar / 10e6);

    printf("speedup=%.1lf\n", tminseq / tminpar);

    

    fp = fopen(filename, "w");

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < m; j++)
        {
            fprintf(fp, "%i ", grid[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    for (i = 0; i < n; i++)
    {
        free(grid[i]);
    }
    free(grid);

    return 0;
}
