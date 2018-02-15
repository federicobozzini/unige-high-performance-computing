#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>
#include "mpi.h"
#include <string.h>
#include <math.h>

// USAGE: mandelbrot_mpi <cols> <rows> <task_size> <x0> <y0> <dx> <dy>
// OUTPUT: <time_spent_in_ms>

#define TRIALS 50
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
    int rows, cols, size, max_iteration, task_size, iMin, iMax, *grid;
    double ttot, tstart, tend, tmin;
    char filename[] = "results/mandelbrot_mpi.dat";
    MPI_Status status;
    int *buffer;
    int me, numinstances;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &numinstances);

    if (argc < 4)
    {
        if (me == 0)
        {
            printf("Usage: mandelbrot_mpi cols rows task_size x0=-2.5 y0=-1 dx=-1 dy=1\n");
        }
        return 1;
    }
    cols = atoi(argv[1]);
    rows = atoi(argv[2]);
    task_size = atoi(argv[3]);

    size = rows * cols;

    max_iteration = 100;
    double xmin = argc > 4 ? atof(argv[4]) : -2.5;
    double ymin = argc > 5 ? atof(argv[5]) : -1;
    double xmax = argc > 6 ? xmin + atof(argv[6]) : 1;
    double ymax = argc > 7 ? ymin + atof(argv[7]) : 1;

    if (xmin >= xmax || ymin >= ymax)
    {
        if (me == 0)
        {
            printf("Usage: mandelbrot_mpi cols rows task_size x0=-2.5 y0=-1 dx=-1 dy=1\n");
        }
        return 1;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (me == 0)
    {
        grid = (int *)malloc(size * sizeof(int));
    }

    buffer = (int *)malloc((ISTR_SIZE + task_size) * sizeof(int));
    for (int k = 0; k < TRIALS; k++)
    {
        if (me == 0)
        {
            int tasks_sent = 0, init_task_num = 2;

            //index of the tasks sent to the workers, it will be increased after every sent message
            int task_idx = 0;

            tmin = 10e10;

            tstart = get_time();

            //The initial number of tasks (init_task_num) is sent to all the workers
            for (int other = 1; other < numinstances; other++)
            {
                for (int t = 0; t < init_task_num; t++)
                {
                    //Checks if no more tasks should be sent
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

            //While the workers are still executing tasks, the coordinator should keep on listening
            while (tasks_sent)
            {
                MPI_Recv(buffer, ISTR_SIZE + task_size, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                tasks_sent--;
                iMin = buffer[0];
                //The results received may be smaller than task_size if the task received is the last one
                iMax = MIN(iMin + task_size, size);
                //The result of a task is copied to the final grid
                memcpy(grid + iMin, buffer + ISTR_SIZE, (iMax - iMin) * sizeof(int));
                //Checks again is no more tasks should be sent
                if (task_idx >= size)
                    continue;
                buffer[0] = task_idx;
                task_idx += task_size;
                MPI_Send(buffer, ISTR_SIZE, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
                tasks_sent++;
            }

            //No more tasks to complete, a final message is sent to stop the workers
            for (int other = 1; other < numinstances; other++)
            {
                MPI_Send(NULL, 0, MPI_INT, other, TAG_KILL, MPI_COMM_WORLD);
            }

            tend = get_time();

            ttot = tend - tstart;

            tmin = MIN(tmin, ttot);
        }
        else
        {
            while (1)
            {
                MPI_Recv(buffer, ISTR_SIZE, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                //Checks if it's the final message
                if (status.MPI_TAG == TAG_KILL)
                    break;
                iMin = buffer[0];
                //Checks if it is the edge case at the end of the grid
                iMax = MIN(iMin + task_size, size);

                //Mandelbrot calculations, parallelized with openMP
#pragma omp parallel for schedule(static)
                for (int i = iMin; i < iMax; i++)
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
                    buffer[i - iMin + ISTR_SIZE] = iteration;
                }

                MPI_Send(buffer, ISTR_SIZE + task_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }
    }
    free(buffer);

    if (me == 0)
    {

        printf("%.2lf\n", tmin / 10e6);

        fp = fopen(filename, "w");

        fprintf(fp, "%.2lf %.2lf %.2lf %.2lf\n", xmin, ymin, xmax - xmin, ymax - ymin);

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
