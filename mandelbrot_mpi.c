#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>
#include "mpi.h"
#include <string.h>

// USAGE: mandelbrot <m> <n>
// OUTPUT: MULTIPLE ROWS IN THE FORMAT <task_size> <time_spent_in_ms>

#define MPI_TRIALS 2
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
    int m, n, size, i, j, k, t, iteration, max_iteration, task_size, ti, jMin, jMax;
    double x0, y0, x, y, xmin, xmax, ymin, ymax, xtemp, ttot, tstart, tend, tminseq, tminpar, tminmpi;
    char filename[] = "mandelbrot_mpi.dat";
    MPI_Status status;
    int *buffer;
    int me, numinstances, init_task_num;
    int max_task_size_length = 10, tssize;
    int *task_sizes;
    int task_params[10] = {1, 2, 5, 10, 20, 50, 100, 200, 500, 1000};

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

    tssize = 0;
    task_sizes = (int *)malloc(max_task_size_length * sizeof(int));
    for (ti = 0; ti < max_task_size_length; ti++)
    {
        if (m/task_params[ti] == 0)
            break;
        tssize++;
        task_sizes[ti] = m/task_params[ti];
    }
    max_iteration = 100;
    xmin = -2.5;
    xmax = 1;
    ymin = -1;
    ymax = 1;

    init_task_num = 4;

    //openMP+MPI

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &numinstances);

    MPI_Barrier(MPI_COMM_WORLD);

    for (ti = 0; ti < tssize; ti++)
    {
        task_size = task_sizes[ti];
        buffer = (int *)malloc((ISTR_SIZE + task_size) * sizeof(int));
        for (k = 0; k < MPI_TRIALS; k++)
        {
            if (me == 0)
            {
                int **grid = (int **)malloc(n * sizeof(int *));
                for (i = 0; i < n; i++)
                {
                    grid[i] = (int *)malloc(m * sizeof(int));
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
                        int task_params[3] = {i, j};
                        j += task_size;
                        if (j >= m)
                        {
                            j = 0;
                            i++;
                        }
                        if (i == n)
                        {
                            other = numinstances;
                            break;
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
                    int r = buffer[0];
                    jMin = buffer[1];
                    jMax = MIN(jMin + task_size, m);
                    int dataSize = jMax - jMin;
                    memcpy(grid[r] + jMin, buffer + ISTR_SIZE, dataSize * sizeof(int));
                    if (i == n)
                        continue;
                    int task_params[3] = {i, j};
                    j += task_size;
                    if (j >= m)
                    {
                        j = 0;
                        i++;
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

                if (k == MPI_TRIALS - 1)
                {

                    printf("%i %.2lf\n", task_size, tminmpi / 10e6);

                    if (ti == tssize - 1)
                    {
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
                    }
                }

                for (i = 0; i < n; i++)
                {
                    free(grid[i]);
                }
                free(grid);
            }

            else
            {
                int *tmp = (int *)malloc(task_size * sizeof(int));
                while (1)
                {
                    MPI_Recv(buffer, ISTR_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    i = buffer[0];
                    if (i < 0)
                        break;
                    jMin = buffer[1];
                    jMax = MIN(jMin + task_size, m);

                    for (j = jMin; j < jMax; j++)
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
                        tmp[j - jMin] = iteration;
                    }
                    int dataSize = jMax - jMin;

                    memcpy(buffer + ISTR_SIZE, tmp, dataSize * sizeof(int));
                    MPI_Send(buffer, ISTR_SIZE + dataSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }

                free(tmp);
            }
        }
        free(buffer);
    }

    MPI_Finalize();

    return 0;
}
