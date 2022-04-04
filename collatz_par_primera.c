/*  SCP, Sistemas de Cómputo Paralelo -- GII (Ingeniería de Computadores)
    Laboratorio de MPI

    collatz_ser.c

***************************************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

#define ZMAX 1000

int collatz(int n)
{
  int iter = 0;

  while (n > 1)
  {
    if ((n % 2) == 1)
      n = 3 * n + 1;
    else
      n = n / 2;

    iter++;
  }
  return (iter);
}

void carga(int iter)
{
  usleep(1000 * iter);
}

int main(int argc, char *argv[])
{
  int n, iter, num_iter = 0, iter_max = 0, z_max = 0, num_iter_final = 0, iter_max_final = 0, z_max_final = 0;
  int pid, npr, n_tareas;

  struct timespec t0, t1;
  double tej, tej_final;

  MPI_Status stats;

  clock_gettime(CLOCK_REALTIME, &t0);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);
  MPI_Comm_size(MPI_COMM_WORLD, &npr);

  // numero de tareas de cada proceso para
  n_tareas = ZMAX / npr;

  for (n = pid * n_tareas; n < (((pid + 1) * n_tareas) - 1); n++)
  {
    iter = collatz(n);
    carga(iter);

    num_iter += iter;
    if (iter > iter_max)
    {
      z_max = n;
      iter_max = iter;
    }
  }

  clock_gettime(CLOCK_REALTIME, &t1);
  tej = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / (double)1e9;
  printf("\n Procesador P%d \n  >> Total: %d iteraciones\n  >> Z_max:   %3d (%3d iteraciones)\n\n",
         pid, num_iter, z_max, iter_max);

  tej *= 1000;
  MPI_Reduce(&num_iter, &num_iter_final, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&tej, &tej_final, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  int info[2];
  if (pid != 0)
  {
    info[0] = z_max;
    info[1] = iter_max;
    MPI_Send(&info[0], 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
  }
  else
  {
    for (n = 0; n < npr - 1; n++)
    {
      MPI_Recv(&info[0], 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &stats);
      if (iter_max_final < info[1])
      {
        iter_max_final = info[1];
        z_max_final = info[0];
      }
    }
    printf("\n TOTAL: \n >> Total: %d iteraciones\n  >> Z_max:   %3d (%3d iteraciones)\n\n  >> Tej:     %1.3f ms\n\n",
           num_iter_final, z_max_final, iter_max_final, tej_final);
  }

  MPI_Finalize();

  return (0);
}
