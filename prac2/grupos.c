/*  SCP, Sistemas de Cómputo Paralelo -- GII (Ingeniería de Computadores)
    Laboratorio de MPI

    grupos.c		-- PARA COMPLETAR

    Repartir 32 procesos en 4 grupos
    Generar un quinto grupo con los procesos 0 de los cuatro anteriores.

    SOLO para 32 procesos
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define N1 4
#define N2 80

int main(int argc, char *argv[])
{
  int pid = 0, i = 0, j = 0, npr = 0;
  int A[N1][N2], T[N2];

  MPI_Status info;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npr);

  // Control de errores
  if (npr != 32)
  {
    printf("This application is meant to be run with 32 MPI processes, not %d.\n", npr);
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  // valores iniciales
  if (pid == 0)
  {
    srand(0);
    for (i = 0; i < N1; i++)
      for (j = 0; j < N2; j++)
        A[i][j] = rand() % 50;
  }

  // 1 -> generar los grupos
  // =======================
  char subcommunicator; // Le asigno un subcommunicator así en los prints quedan más claros los grupos
  int colour = 0;
  int key = 0;
  if (pid % 4 == 0)
  {
    subcommunicator = 'A';
    colour = 0;
    key = pid;
  }
  else if (pid % 4 == 1)
  {
    subcommunicator = 'B';
    colour = 1;
    key = pid;
  }
  else if (pid % 4 == 2)
  {

    subcommunicator = 'C';
    colour = 2;
    key = pid;
  }
  else
  {
    subcommunicator = 'D';
    colour = 3;
    key = pid;
  }

  // Split de global communicator
  MPI_Comm new_comm;
  MPI_Comm_split(MPI_COMM_WORLD, colour, key, &new_comm);

  // Get my rank in the new communicator
  int new_pid = 0;
  MPI_Comm_rank(new_comm, &new_pid);

  // Print my new rank and new communicator
  printf("[MPI proceso %d] Ahora soy el proceso MPI %d del subcommunicator %c.\n", pid, new_pid, subcommunicator);

  // 2 -> repartir la matriz a los "jefes" de grupo
  // ==============================================
  
  sleep(1); //Para respetar los prints, se puede quitar

  // El P0 global se encarga de repartir la matriz
  if (pid == 0)
  {
    printf("\n\nMATRIZ A >>\n\n");
    for (i = 0; i < N1; i++)
    {
      for (j = 0; j < 10; j++)
        printf("%2d", A[i][j]);
      printf("\n");
    }
    printf("\n");

    //Enviar las filas a todos los procesos cabecera
    for (i = 0; i < N1; i++)
    {
      MPI_Send(&A[i][0], N2, MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    MPI_Recv(&T, N2, MPI_INT, 0, 0, MPI_COMM_WORLD, &info);
  }
  else if (new_pid == 0) //Los procesos cabecera recogen las filas
  {
    MPI_Recv(&T, N2, MPI_INT, 0, 0, MPI_COMM_WORLD, &info);
  }

  // 3 -> repartir los datos recibidos entre los miembros del grupo
  //      procesarlos (cualquier operacion)
  //      recogerlos nuevamente en la cabecera
  // ==============================================================

  int X[10];
  MPI_Scatter(T, 10, MPI_INT, X, 10, MPI_INT, 0, new_comm); //Los procesos cabecera reparten los trozos de la fila
  srand(0);
  for (i = 0; i < 10; i++)
  {
    X[i] = rand() % (pid + 1); //Operacion aleatoria elegida
  }
  MPI_Gather(X, 10, MPI_INT, T, 10, MPI_INT, 0, new_comm); //Se devuelven los trozos de fila a los procesos cabecera

  // 4 -> operacion colectiva entre los jefes de grupo: sumar todos los elementos
  //      de cada grupo y calcular el minimo
  // ============================================================================

  int minimo = 0;
  int suma = 0;
  if (new_pid == 0)
  {
    colour = 5; //Para crear el comunicador de los procesos cabecera
    for (i = 0; i < N2; i++)
    {
      suma += T[i]; //La suma de los elementos
    }
  }

  // Split de global communicator para crear el comunicador de las cabeceras
  MPI_Comm new_comm_cabeceras;
  MPI_Comm_split(MPI_COMM_WORLD, colour, key, &new_comm_cabeceras);

  //El minimo de todas las sumas de las cabeceras
  MPI_Allreduce(&suma, &minimo, 1, MPI_INT, MPI_MIN, new_comm_cabeceras);

  // imprimir resultados
  if (new_pid == 0)
    printf("[MPI process %d] I am now MPI process %d in subcommunicator %c. Mi suma ha sido %d y el minimo es %d\n", pid, new_pid, subcommunicator, suma, minimo);

  MPI_Finalize();

  return (0);
}
