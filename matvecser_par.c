/*  SCP, Sistemas de Cómputo Paralelo -- GII (Ingeniería de Computadores)
    Laboratorio de MPI

    matvecser.c

    Calcula 	C() = A()() * B()
                D() = A()() * C()
                x = raiz (suma (C()*C() + D()*D())
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
  int N = 0, i = 0, j = 0, k = 0, npr = 0, pid = 0, count = 0, tag = 0, origen = 0, *aux, sum = 0;
  float *Am, *B, *C, *D, *rec_buf, c_cuadrado = 0, d_cuadrado = 0, resultado_c = 0, resultado_d = 0, x;

  MPI_Status info;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npr);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    if (pid == 0){
      printf("\n Longitud de los vectores (<1000) = \n");
      scanf("%d", &N);
    }

    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);


  /* reserva de memoria */
  Am = (float *)malloc(N * N * sizeof(double));
  aux = (int *)malloc(npr * sizeof(int)); //vector para calcular cuantas filas del vector Am van a cada P
  B = (float *)malloc(N * sizeof(float));
  C = (float *)malloc(N * sizeof(float));
  D = (float *)malloc(N * sizeof(float));
  float *cbuf = (float *)malloc(N * sizeof(float));
  float *dbuf = (float *)malloc(N * sizeof(float));

  if (pid == 0)
  {

    /* inicializar variables */
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
        Am[i * N + j] = (N - i) * 0.1 / N;
      B[i] = i * 0.05 / N;
    }
  }

  // Calculos para enviar a los diferentes procesos con Scatterv
  int *sendcounts = malloc(sizeof(int) * npr);
  int *displs = malloc(sizeof(int) * npr);
  int resto = N % npr;
  // calculate send counts and displacements
  for (i = 0; i < npr; i++)
  {
    aux[i] = N / npr;
    sendcounts[i] = N * aux[i];
    if (resto > 0)
    {
      sendcounts[i] += N;
      resto--;
      aux[i]++;
    }
    displs[i] = sum;
    sum += sendcounts[i];
  }

  // Broadcast para enviar B
  MPI_Bcast(B, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  // Tratamiento de Scatterv
  if (pid == 0)
  {
    rec_buf = malloc(sendcounts[pid]*sizeof(float));
    MPI_Scatterv(Am, sendcounts, displs, MPI_FLOAT, rec_buf, sendcounts[pid], MPI_FLOAT, 0, MPI_COMM_WORLD);
  }
  else
  {
    rec_buf = malloc(sendcounts[pid]*sizeof(float));
    MPI_Scatterv(NULL, NULL, NULL, MPI_FLOAT, rec_buf, sendcounts[pid], MPI_FLOAT, 0, MPI_COMM_WORLD);
  }

  // Calculo de C
  j = -1;
  for (i = 0; i < sendcounts[pid]; i++)
  {
    if ((i % N) == 0)
    {
      k = 0;
      j++;
      cbuf[j] = 0;
    }
    cbuf[j] += (rec_buf[i] * B[k]);
    k++;
  }

  // Calculamos el C[]^2
  c_cuadrado = 0;
  for (i = 0; i < aux[pid]; i++)
  {

    c_cuadrado += (cbuf[i] * cbuf[i]);
  }
  resultado_c = 0;
  // Suma de todos los C[]^2
  MPI_Reduce(&c_cuadrado, &resultado_c, aux[pid], MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

  // Condicion para enviar el Gatherv y poder recoger C
  if (pid == 0)
  {
    // Recogida de C en P0
    int *displs2 = malloc(sizeof(int) * npr);
    sum = 0;
    for (i = 0; i < npr; i++)
    {
      displs2[i] = sum;
      sum += aux[i];
    }

    MPI_Gatherv(cbuf, aux[pid], MPI_FLOAT, C, aux, displs2, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }
  else
  {
    MPI_Gatherv(cbuf, aux[pid], MPI_FLOAT, NULL, NULL, NULL, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }

  // Broadcast para enviar C
  MPI_Bcast(C, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  // Calculo de D
  j = -1;
  for (i = 0; i < sendcounts[pid]; i++)
  {
    if ((i % N) == 0)
    {
      k = 0;
      j++;
      dbuf[j] = 0;
    }
    dbuf[j] += (rec_buf[i] * C[k]);
    k++;
  }

  // Calculamos el D[]^2
  d_cuadrado = 0;
  for (i = 0; i < aux[pid]; i++)
  {

    d_cuadrado += (dbuf[i] * dbuf[i]);
  }
  resultado_d = 0;
  MPI_Reduce(&d_cuadrado, &resultado_d, aux[pid], MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);

  // Condicion para enviar el Gatherv y poder recoger D
  if (pid == 0)
  {
    // Recogida de D en P0
    int *displs2 = malloc(sizeof(int) * npr);
    sum = 0;
    for (i = 0; i < npr; i++)
    {
      displs2[i] = sum;
      sum += aux[i];
    }

    MPI_Gatherv(dbuf, aux[pid], MPI_FLOAT, D, aux, displs2, MPI_FLOAT, 0, MPI_COMM_WORLD);
    // Calculo de la raiz y printeos
    x = sqrt(resultado_d + resultado_c);
    printf("\n\n   x = %1.3f \n", x);
    printf("   D[0] = %1.3f, D[N/2] = %1.3f, D[N-1] = %1.3f\n\n", D[0], D[N / 2], D[N - 1]);
  }
  else
  {
    MPI_Gatherv(dbuf, aux[pid], MPI_FLOAT, NULL, NULL, NULL, MPI_FLOAT, 0, MPI_COMM_WORLD);
  }

  MPI_Finalize();

  free(Am);
  free(B);
  free(C);
  free(D);
  free(aux);
  free(cbuf);
  free(dbuf);
  return (0);
}
