#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>
#include "mpi.h"
#include <string.h>
#include <math.h>

// USAGE: mandelbrot <cols> <rows>
// OUTPUT: MULTIPLE ROWS IN THE FORMAT <task_size> <time_spent_in_ms>

#define TRIALS 10
#define ISTR_SIZE 1
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
    int rows, cols, i, j, px, py, k, t, iteration, max_iteration, task_size, ti, jMin, jMax, iMin, iMax;
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
            printf("Usage: mandelbrot_mpi cols rows\n");
        }
        return 1;
    }
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);
    int size = rows * cols;
    int ti_num = 10;

    if (rows < 100 || cols < 100)
    {
        if (me == 0)
        {
            printf("Error: cols and rows must be > %i\n", 100);
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

    int max_ti = (int)sqrt(rows * cols / (numinstances - 1));
    int ti_step = max_ti / ti_num;
    max_ti = ti_num * ti_step;

    for (ti = ti_step; ti <= max_ti; ti += ti_step)
    {
        task_size = ti * ti;
        buffer = (int *)malloc((ISTR_SIZE + task_size) * sizeof(int));
        for (k = 0; k < TRIALS; k++)
        {
            if (me == 0)
            {
                int *grid = (int *)malloc(size * sizeof(int));

                int other, tasks_sent = 0;

                i = 0;

                tminmpi = 10e10;

                tstart = get_time();

                for (other = 1; other < numinstances; other++)
                {
                    for (t = 0; t < init_task_num; t++)
                    {
                        if (i >= size)
                        {
                            other = numinstances;
                            break;
                        }
                        buffer[0] = i;
                        MPI_Send(buffer, ISTR_SIZE, MPI_INT, other, 0, MPI_COMM_WORLD);
                        i += task_size;
                        tasks_sent++;
                    }
                }

                while (tasks_sent)
                {
                    MPI_Recv(buffer, ISTR_SIZE + task_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    tasks_sent--;
                    iMin = buffer[0];
                    iMax = MIN(iMin + task_size, size);
                    memcpy(grid+iMin, buffer+ISTR_SIZE, (iMax - iMin)* sizeof(int));
                    if (i >= size)
                        continue;
                    buffer[0] = i;
                    i += task_size;
                    MPI_Send(buffer, ISTR_SIZE, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                    tasks_sent++;
                }

                for (other = 1; other < numinstances; other++)
                {
                    buffer[0] = -1;
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
                                fprintf(fp, "%i ", grid[rows * i + j]);
                            }
                            fprintf(fp, "\n");
                        }

                        fclose(fp);
                    }
                }
                free(grid);
            }

            else
            {
                while (1)
                {
                    MPI_Recv(buffer, ISTR_SIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    if (buffer[0] < 0)
                        break;
                    iMin = buffer[0];
                    iMax = MIN(iMin + task_size, size);

                    int index = ISTR_SIZE;

#pragma omp parallel for private(i, px, py, x0, y0, x, y, iteration, xtemp) schedule(static) default(shared) firstprivate(index) 
                    for (i = iMin; i < iMax; i++)
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
                        buffer[index] = iteration;
                        index++;
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
