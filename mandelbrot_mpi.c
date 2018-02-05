#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>
#include "mpi.h"
#include <string.h>
#include <math.h>

// USAGE: mandelbrot <cols> <rows>
// OUTPUT: MULTIPLE ROWS IN THE FORMAT <task_size> <time_spent_in_ms>

#define TRIALS 3
#define ISTR_SIZE 2
#define BUFFER_SIZE (ISTR_SIZE + MAX_DATA_SIZE)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

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
    int rows, cols, i, j, k, t, iteration, max_iteration, task_size, ti, jMin, jMax, iMin, iMax, t_step, rowsToCopy, chunkSize;
    double x0, y0, x, y, xmin, xmax, ymin, ymax, xtemp, ttot, tstart, tend, tminseq, tminpar, tminmpi;
    char filename[] = "mandelbrot_mpi.dat";
    MPI_Status status;
    int *buffer;
    int me, numinstances, init_task_num;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &numinstances);

    if (argc < 3)
    {
        if (me == 0)
        {
            printf("Usage: mandelbrot cols rows\n");
        }
        return 1;
    }
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);

    if (rows < 2 || cols < 2)
    {
        if (me == 0)
        {
            printf("Error: cols and rows must be > 2\n");
        }
        return 1;
    }

    max_iteration = 100;
    xmin = -2.5;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    init_task_num = 4;

    MPI_Barrier(MPI_COMM_WORLD);

    t_step = 20;

    int max_ti = (int) sqrt(rows * cols / init_task_num / (numinstances - 1)) / t_step * t_step;
    for (ti = t_step; ti <= max_ti; ti += t_step)
    {
        task_size = ti * ti;
        buffer = (int *)malloc((ISTR_SIZE + task_size) * sizeof(int));
        for (k = 0; k < TRIALS; k++)
        {
            if (me == 0)
            {
                int **grid = (int **)malloc(cols * sizeof(int *));
                for (i = 0; i < cols; i++)
                {
                    grid[i] = (int *)malloc(rows * sizeof(int));
                }
                int other, tasks_sent = 0;

                i = 0;
                j = 0;

                tminmpi = 10e10;

                tstart = get_time();

                for (other = 1; other < numinstances; other++)
                {
                    for (t = 0; t < init_task_num; t++)
                    {
                        if (i >= cols)
                        {
                            other = numinstances;
                            break;
                        }
                        if (j == rows)
                        {
                            j = 0;
                        }
                        int task_params[3] = {i, j};
                        j += ti;
                        if (j >= rows)
                        {
                            j = rows;
                            i += ti;
                        }
                        memcpy(buffer, task_params, ISTR_SIZE * sizeof(int));
                        MPI_Send(buffer, ISTR_SIZE, MPI_INT, other, 0, MPI_COMM_WORLD);
                        tasks_sent++;
                    }
                }

                while (tasks_sent)
                {
                    MPI_Recv(buffer, ISTR_SIZE + task_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    tasks_sent--;
                    iMin = buffer[0];
                    iMax = MIN(iMin + ti, cols);
                    jMin = buffer[1];
                    jMax = MIN(jMin + ti, rows);
                    chunkSize = jMax - jMin;
                    rowsToCopy = iMax - iMin;
                    for (int r = 0; r < rowsToCopy; r++)
                    {
                        memcpy(grid[r + iMin] + jMin, buffer + ISTR_SIZE + r * chunkSize, chunkSize * sizeof(int));
                    }
                    if (i >= cols)
                        continue;
                    if (j == rows)
                    {
                        j = 0;
                    }
                    int task_params[3] = {i, j};
                    j += ti;
                    if (j >= rows)
                    {
                        j = rows;
                        i += ti;
                    }
                    memcpy(buffer, task_params, ISTR_SIZE * sizeof(int));
                    MPI_Send(buffer, ISTR_SIZE, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                    tasks_sent++;
                }

                for (other = 1; other < numinstances; other++)
                {
                    int finished[1] = {-1};
                    memcpy(buffer, finished, sizeof(int));
                    MPI_Send(buffer, ISTR_SIZE, MPI_INT, other, 0, MPI_COMM_WORLD);
                }

                tend = get_time();

                ttot = tend - tstart;

                if (ttot < tminmpi)
                    tminmpi = ttot;

                if (k == TRIALS - 1)
                {

                    printf("%i %.2lf\n", task_size, tminmpi / 10e6);

                    if (ti == max_ti)
                    {
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
                    }
                }

                for (i = 0; i < cols; i++)
                {
                    free(grid[i]);
                }
                free(grid);
            }

            else
            {
                while (1)
                {
                    MPI_Recv(buffer, ISTR_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    iMin = buffer[0];
                    iMax = MIN(iMin + ti, cols);
                    if (iMin < 0)
                        break;
                    jMin = buffer[1];
                    jMax = MIN(jMin + ti, rows);
                    rowsToCopy = iMax - iMin;
                    chunkSize = jMax - jMin;

                    for (i = iMin; i < iMax; i++)
                    {
                        for (j = jMin; j < jMax; j++)
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
                            int index = (i - iMin) * chunkSize + (j - jMin) + ISTR_SIZE;
                            buffer[index] = iteration;
                        }
                    }
                    MPI_Send(buffer, ISTR_SIZE + task_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
            }
        }
        free(buffer);
    }

    MPI_Finalize();

    return 0;
}
