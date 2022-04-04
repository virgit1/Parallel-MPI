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
  int n, iter, i, flag;
  int iter_max[2] = {0, 0};
  int pid, npr, origen, tag, vacio, count;
  int resul[2] = {0, 0};
  int num_n = 0, max = 0;
  int seguir_worker = 1;

  MPI_Status info;
  MPI_Request req;

  struct timespec t0, t1;
  double tej;

  clock_gettime(CLOCK_REALTIME, &t0);

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);
  MPI_Comm_size(MPI_COMM_WORLD, &npr);

  int *num_worker = calloc(npr, sizeof(int)); // Vector donde meto cuantos numeros ha calculado cada PX
  int final_mssg = npr - 1;

  if (pid == 0) // P0 manager
  {
    n = 1;
    while (n <= 1000 || final_mssg > 0) // Para decidir cuando acaba el proceso,
    {                                   // final_mssg es para cuando se hayan recibido los resultados de cada proceso y poder saber que numero ha tardado mas
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &info);
      tag = info.MPI_TAG;
      origen = info.MPI_SOURCE;
      switch (tag)
      {
      case 0:         // caso en el que los workers quieren otro numero
        if (n > 1000) // Si se han hecho los 1000 numeros se envia un ultimo mensaje con tag 1 para finalizar
        {
          MPI_Recv(&vacio, 0, MPI_INT, origen, tag, MPI_COMM_WORLD, &info);
          MPI_Send(&vacio, 0, MPI_INT, origen, 1, MPI_COMM_WORLD);
        }
        else // si no, se envia otro numero
        {
          MPI_Recv(&vacio, 0, MPI_INT, origen, tag, MPI_COMM_WORLD, &info);
          MPI_Send(&n, 1, MPI_INT, origen, 0, MPI_COMM_WORLD);
          num_worker[origen]++;
          n++;
        }
        break;
      case 1: // Caso en el que los procesos han acabado y P0 recoge los resultados(me he fijado que en este case nunca entra)
        MPI_Get_count(&info, MPI_INT, &count);
        MPI_Recv(resul, count, MPI_INT, origen, tag, MPI_COMM_WORLD, &info);
        if (resul[0] > max) // Calculo de el numero con más iteracciones
        {
          max = resul[0];
          num_n = resul[1];
        }
        final_mssg--;
        break;
      default:
        break;
      }
    }
    printf("/********PROCESADOR 0********/\n");
    for (i = 1; i < npr; i++)
    {

      printf("P%d ha procesado %d numeros\n", i, num_worker[i]);
    }
    printf("\n Y el numero que mas iteracciones ha necesitado es %d con %d iteracciones\n", num_n, max);
  }
  else // Los otros PX
  {

    MPI_Send(&vacio, 0, MPI_INT, 0, 0, MPI_COMM_WORLD); // Primer envio para decir que estan listos
    while (seguir_worker)
    {
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &info);
      tag = info.MPI_TAG;
      origen = info.MPI_SOURCE;
      switch (tag)
      {
      case 0: // Caso en el que reciben el numero y hacen los calculos
        MPI_Recv(&n, 1, MPI_INT, origen, tag, MPI_COMM_WORLD, &info);
        iter = collatz(n);
        carga(iter);
        if (iter > iter_max[0])
        {
          iter_max[0] = iter;
          iter_max[1] = n;
        }
        MPI_Send(&vacio, 0, MPI_INT, origen, 0, MPI_COMM_WORLD);
        break;
      case 1: // Caso en el que reciben el mensaje de finalizacion y mandan el resultado(me he dado cuenta de que este mensaje no llega)
        MPI_Recv(&vacio, 0, MPI_INT, origen, tag, MPI_COMM_WORLD, &info);
        MPI_Send(iter_max, 2, MPI_INT, origen, 1, MPI_COMM_WORLD);
        seguir_worker = 0;
        break;
      default:
        break;
      }
    }
  }
  if (pid == 0)
  {
    clock_gettime(CLOCK_REALTIME, &t1);
    tej = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / (double)1e9;
    printf("\n\n  >> Tej:     %1.3f ms\n\n", tej * 1000);
  }

  MPI_Finalize();

  free(num_worker);
  return (0);
}
