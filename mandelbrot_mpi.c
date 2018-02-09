#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>
#include "mpi.h"
#include <string.h>
#include <math.h>

// USAGE: mandelbrot_mpi <cols> <rows>
// OUTPUT: MULTIPLE ROWS IN THE FORMAT <task_size> <time_spent_in_ms>

#define TRIALS 10
#define ISTR_SIZE 1
#define BUFFER_SIZE (ISTR_SIZE + MAX_DATA_SIZE)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define TAG_KILL 1

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
    int rows, cols, px, py, k, t, iteration, max_iteration, task_size, iMin, iMax;
    double x0, y0, x, y, xmin, xmax, ymin, ymax, xtemp, ttot, tstart, tend, tmin;
    char filename[] = "mandelbrot_mpi.dat";
    MPI_Status status;
    int *buffer;
    int me, numinstances;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &numinstances);

    if (argc < 3)
    {
        if (me == 0)
        {
            printf("Usage: mandelbrot_mpi cols rows\n");
        }
        return 1;
    }
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);
    int size = rows * cols;
    int *grid;

    max_iteration = 100;
    xmin = -2.5;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    MPI_Barrier(MPI_COMM_WORLD);

    int ti_step = size / 1000;

    if (me == 0)
        grid = (int *)malloc(size * sizeof(int));

    for (task_size = ti_step; task_size < size / (numinstances - 1); task_size += ti_step)
    {
        buffer = (int *)malloc((ISTR_SIZE + task_size) * sizeof(int));
        for (k = 0; k < TRIALS; k++)
        {
            if (me == 0)
            {
                int other, tasks_sent = 0, init_task_num = 2;

                int task_idx = 0;

                tmin = 10e10;

                tstart = get_time();

                for (other = 1; other < numinstances; other++)
                {
                    for (t = 0; t < init_task_num; t++)
                    {
                        if (task_idx >= size)
                        {
                            other = numinstances;
                            break;
                        }
                        buffer[0] = task_idx;
                        MPI_Send(buffer, ISTR_SIZE, MPI_INT, other, 0, MPI_COMM_WORLD);
                        task_idx += task_size;
                        tasks_sent++;
                    }
                }

                while (tasks_sent)
                {
                    MPI_Recv(buffer, ISTR_SIZE + task_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    tasks_sent--;
                    iMin = buffer[0];
                    iMax = MIN(iMin + task_size, size);
                    memcpy(grid + iMin, buffer + ISTR_SIZE, (iMax - iMin) * sizeof(int));
                    if (task_idx >= size)
                        continue;
                    buffer[0] = task_idx;
                    task_idx += task_size;
                    MPI_Send(buffer, ISTR_SIZE, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                    tasks_sent++;
                }

                for (other = 1; other < numinstances; other++)
                {
                    MPI_Send(NULL, 0, MPI_INT, other, TAG_KILL, MPI_COMM_WORLD);
                }

                tend = get_time();

                ttot = tend - tstart;

                if (ttot < tmin)
                    tmin = ttot;

                if (k == TRIALS - 1)
                    printf("%i %.2lf\n", task_size, tmin / 10e6);
            }

            else
            {
                while (1)
                {
                    MPI_Recv(buffer, ISTR_SIZE, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    if (status.MPI_TAG == TAG_KILL)
                        break;
                    iMin = buffer[0];
                    iMax = MIN(iMin + task_size, size);

                    for (int i = iMin; i < iMax; i++)
                    {
                        px = i % rows;
                        py = i / rows;
                        x0 = (double)px / (rows - 1) * (xmax - xmin) + xmin;
                        y0 = (double)py / (cols - 1) * (ymax - ymin) + ymin;
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
                        buffer[i - iMin + ISTR_SIZE] = iteration;
                    }

                    MPI_Send(buffer, ISTR_SIZE + task_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
            }
        }
        free(buffer);
    }

    if (me == 0)
    {
        fp = fopen(filename, "w");

        for (int i = 0; i < cols; i++)
        {
            for (int j = 0; j < rows; j++)
            {
                fprintf(fp, "%i ", grid[rows * i + j]);
            }
            fprintf(fp, "\n");
        }

        fclose(fp);
        free(grid);
    }

    MPI_Finalize();

    return 0;
}
