#include "main.h"

double phi(double x, double y, double z) {
  return x*x + y*y + z*z;
}

double ro(double x, double y, double z) {
  return 6 - a * phi(x, y, z);
}

double F(double *M, double i, double j, double k, double x, double y, double z) {
  double
    Fx = (M[iter(i+1, j, k)] + M[iter(i-1, j, k)])/(hx*hx),
    Fy = (M[iter(i, j+1, k)] + M[iter(i, j-1, k)])/(hy*hy),
    Fz = (M[iter(i, j, k+1)] + M[iter(i, j, k-1)])/(hz*hz);

  return B * (Fx + Fy + Fz - ro(x, y, z));
}


double jacobi() {
  int rank, processes_number;

  MPI_Comm_size(MPI_COMM_WORLD, &processes_number); // получаем количество запущенных процессов
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // получаем id процесса

  //инициализирует начальную матрицу
  double *initial_data = (double*)malloc(sizeof(double)*Nx * Ny * Nz);
  __ROOT
  for (int i = 0; i < Nx; i++) {
    for (int j = 0; j < Ny; j++) {
      for (int k = 0; k < Nz; k++) {
        if (i == 0 || i == Nx-1 || j == 0 || j == Ny-1 || k == 0 || k == Nz-1 ) {
          double
            x = X1 + i * hx,
            y = Y1 + j * hy,
            z = Z1 + k * hz;
          initial_data[iter(i, j, k)] = phi(x, y, z);
        } else {
          initial_data[iter(i, j, k)] = phi_0;
        }
      }
    }
  }

  // инициализируем данные для разбиения
  int *sendcounts, *displs;
  __ROOT
  {
    sendcounts = (int*)malloc(sizeof(int) * processes_number);
    displs     = (int*)malloc(sizeof(int) * processes_number);
    
    int basic_slice_size = Nx / processes_number;
    int remainder        = Nx % processes_number;

    int shift = 0;

    for (int i = 0; i < processes_number; i++) {
      sendcounts[i] = basic_slice_size + (i < remainder ? 1 : 0);
      displs[i] = shift;
      shift += sendcounts[i];
    }
  }

  int start_border_offset = (rank != 0 ? 1 : 0);
  int end_border_offset   = (rank != processes_number - 1 ? 1 : 0);

  int slice_size  = Nx / processes_number + (rank < Nx % processes_number ? 1 : 0); // размер под данные внутри блока
  int border_size = start_border_offset + end_border_offset; // размер под границы

  double *M_old = (double*)malloc(sizeof(double) * (slice_size + border_size) * Ny*Nz);
  double *M     = (double*)malloc(sizeof(double) * (slice_size + border_size) * Ny*Nz);

  MPI_Scatterv(
    initial_data, sendcounts, displs, MPI_DOUBLE,
    M_old + start_border_offset, slice_size, MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD
  ); // Разбиение данных

  int
    x_start = start_border_offset*2 + (rank == 0 ? 1 : 0),
    x_end   = start_border_offset + slice_size - 1;

  int iteration = 0;
  double M_max;

  double x, y, z;
  do {
    M_max = -1;
    MPI_Request
      previous_send, previous_recive,
      next_send, next_recive;

    // отправляем соседям значения для их границ и получаем для своих
    if (start_border_offset) {
      MPI_Isend(M_old + start_border_offset*Ny*Nz, Ny*Nz, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &previous_send); // отправляем левому
      MPI_Irecv(M_old, Ny*Nz, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &previous_recive); // получаем от левого
    }
    if (end_border_offset) {
      MPI_Isend(M_old + (start_border_offset + slice_size - 1) * Ny*Nz, Ny*Nz, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &next_send); // отправляем правому
      MPI_Irecv(M_old + (start_border_offset + slice_size) * Ny*Nz, Ny*Nz, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &next_recive); // получаем от правого
    }

    // обрабатываем значения внутри (без границ)
    for (int i = x_start; i < x_end; i++) {
      x = X1 + (i + rank*(Nx / processes_number) + min(Nx % processes_number, rank)) * hx;
      for (int j = 1; j < Ny - 1; j++) {
        y = Y1 + j * hy;
        for (int k = 1; k < Nz - 1; k++) {
          z = Z1 + k * hz;
          M[iter(i, j, k)] = F(M_old, i, j, k, x, y, z);
          M_max = max(M_max, fabs(M_old[iter(i, j, k)] - M[iter(i, j, k)]));
        }
      }
    }

    // ожидаем получения значений на границах
    if (start_border_offset) {
      MPI_Wait(&previous_send, MPI_STATUSES_IGNORE);
      MPI_Wait(&previous_recive, MPI_STATUSES_IGNORE);
    }
    if (end_border_offset) {
      MPI_Wait(&next_send, MPI_STATUSES_IGNORE);
      MPI_Wait(&next_recive, MPI_STATUSES_IGNORE);
    }

    // обрабатываем значения на границах
    if (start_border_offset) {
      int i = 1;
      x = X1 + (i + rank*(Nx / processes_number) + min(Nx % processes_number, rank)) * hx;
      for (int j = 1; j < Ny - 1; j++) {
        y = Y1 + j * hy;
        for (int k = 1; k < Nz - 1; k++) {
          z = Z1 + k * hz;
          M[iter(i, j, k)] = F(M_old, i, j, k, x, y, z);
          M_max = max(M_max, fabs(M_old[iter(i, j, k)] - M[iter(i, j, k)]));
        }
      }
    }
    if (end_border_offset) {
      int i = start_border_offset + slice_size - 1;
      x = X1 + (i + rank*(Nx / processes_number) + min(Nx % processes_number, rank)) * hx;
      for (int j = 1; j < Ny - 1; j++) {
        y = Y1 + j * hy;
        for (int k = 1; k < Nz - 1; k++) {
          z = Z1 + k * hz;
          M[iter(i, j, k)] = F(M_old, i, j, k, x, y, z);
          M_max = max(M_max, fabs(M_old[iter(i, j, k)] - M[iter(i, j, k)]));
        }
      }
    }

    // находим M_max среди всех процессов
    MPI_Allreduce(MPI_IN_PLACE, &M_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    memcpy(M_old, M, sizeof(double) * (start_border_offset + slice_size + end_border_offset) * Ny * Nz);
    iteration++;
  } while (M_max > EPS);
  
  free(M_old);
  free(M);
  free(initial_data);
  free(sendcounts);
  free(displs);

  return M_max;
}


int main(int argc, char* argv[]) {
  int rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // получаем id процесса


  MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем начало
  double start = MPI_Wtime();

  double phi_max = jacobi();

  MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем завершение работы
  double end = MPI_Wtime();
  
  MPI_Finalize();

  __ROOT
  {
    printf("Δ = %e\n", phi_max);
    printf("time = %f\n", end - start);
  }

  return 0;
}