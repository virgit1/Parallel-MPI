/*  SCP, Sistemas de Cómputo Paralelo -- GII (Ingeniería de Computadores)
    Laboratorio de MPI

    scatter2D.c		--PARA COMPLETAR

    Reparto 2D de una matriz de 12x12
    - definicion del tipo bloque 2D
    - reparto mediante un bucle de envios
    - reparto mediante un scatter (tras el resize del tipo)

    EJECUTAR con 4, 9 o 16 procesos (bloques de 6x6, 4x4 o 3x3)
************************************************************************/

#include <mpi.h>
#include <stdio.h>

#define N 12 //  divisible por 2, 3 y 4

int main(int argc, char *argv[])
{
  int pid = 0, i = 0, j = 0, A[N][N];
  int npr = 0, Kbl = 0, Kdim = 0, *dis, n_fil = 0, multiplo_kdim = 0;

  MPI_Datatype BLOQUE, col, coltype;
  MPI_Status info;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npr);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  //Inicializar a -1 matriz A
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
      A[i][j] = -1;

  // P0 Inicializa A
  if (pid == 0)
  {
    srand(0);
    for (i = 0; i < N; i++)
      for (j = 0; j < N; j++)
        A[i][j] = rand() % 9;

    printf("\n P0 > A\n");
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
        printf("%2d", A[i][j]);
      printf("\n");
    }
  }

  // 1. Definicion de un bloque 2D

  Kbl = sqrt((N * N) / npr); //Kbl
  Kdim = (N / Kbl); //Kdim

  dis = malloc(sizeof(int) * 2); // direccion de comienzo

  //Operaciones para saber en que fila esta el trozo de bloque
  n_fil = 0;
  multiplo_kdim = Kdim;
  while (pid >= multiplo_kdim)
  {
    n_fil++;
    multiplo_kdim = (Kdim * (n_fil + 1));
  }

  dis[0] = (Kbl * n_fil); //fila
  dis[1] = (Kbl * (pid % Kdim)); //columna
  int tam_N[2] = {N, N}; //Dimensiones de la matriz globlal
  int tam[2] = {Kbl, Kbl}; //Dimensiones del bloque a repartir

  MPI_Type_create_subarray(2, tam_N, tam, dis, MPI_ORDER_C, MPI_INT, &BLOQUE);
  MPI_Type_commit(&BLOQUE);

  // 2. Repartir A por bloques, mediante un bucle de envios; recibir los datos en un buffer
  // Imprimir en cada proceso los datos recibidos

  //Reparto de la matriz por bloques realizado por P0
  int T[Kbl][Kbl];
  if (pid == 0)
  {
    for (i = 1; i < npr; i++)
    {
      n_fil = 0;
      multiplo_kdim = Kdim;
      while (i >= multiplo_kdim) //calculo de la fila dependiendo de i(pid del procesador)
      {
        n_fil++;
        multiplo_kdim = (Kdim * (n_fil + 1));
      }
      dis[0] = (Kbl * n_fil);
      dis[1] = (Kbl * (i % Kdim));
      MPI_Send(&(A[dis[0]][dis[1]]), 1, BLOQUE, i, 0, MPI_COMM_WORLD);
    }
    sleep(1); //Sleep para ordenar mejor los prints
    printf("\n P0 > T VERSION SEND-RECV\n");
    for (i = 0; i < Kbl; i++)
    {
      for (j = 0; j < Kbl; j++)
        printf("%2d", A[i][j]);
      printf("\n");
    }
  }
  else
  {
    //Recepcion en T del bloque
    MPI_Recv(&(T[0][0]), Kbl * Kbl, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("\n P%d > T VERSION SEND-RECV\n", pid);
    for (i = 0; i < Kbl; i++)
    {
      for (j = 0; j < Kbl; j++)
        printf("%2d", T[i][j]);
      printf("\n");
    }
  }

  //Inicializamos el bloque T a -1 para comprobar que la 
  //parte 3 del ejercicio la hacemos bien
  for (i = 0; i < Kbl; i++)
    for (j = 0; j < Kbl; j++)
      T[i][j] = -1;

  // 3. Repartir A por bloques mediante un scatter (tras cambiar la extensión del tipo)
  // Recibir los datos en la matriz A en forma de bloque 2D, en la posición correspondiente

  //resized para poder coger las columnas contiguas
  MPI_Type_create_resized(BLOQUE, 0, Kbl * sizeof(int), &coltype);
  MPI_Type_commit(&coltype);

  //Calculos del counts y dipls para hacer el Scatterv 
  int *counts = malloc(npr * sizeof(int));
  int *displs = malloc(npr * sizeof(int));
  for (i = 0; i < npr; i++)
  {
    counts[i] = 1;
  }

  int disp = 0;
  for (i = 0; i < Kdim; i++)
  {
    for (j = 0; j < Kdim; j++)
    {
      displs[i * Kdim + j] = disp;
      disp++;
    }
    disp += (Kbl - 1) * Kdim;
  }

  MPI_Scatterv(&(A[0][0]), counts, displs, coltype, &(T[0][0]), Kbl * Kbl, MPI_INT, 0, MPI_COMM_WORLD);

  // Imprimir en cada proceso la nueva matriz A
  if (pid == 0) //El if es porque si no los prints se pisan unos a otros 
                //por eso imprimos solo P0, pero se puede quitar 
                //o modificar si se quiere ver otro PX
  {
    printf("\n P%d > T VERSION CON SCATTER\n", pid);
    for (i = 0; i < Kbl; i++)
    {
      for (j = 0; j < Kbl; j++)
        printf("%2d", T[i][j]);
      printf("\n");
    }
  }

  MPI_Type_free(&BLOQUE);

  MPI_Finalize();

  return (0);
}
