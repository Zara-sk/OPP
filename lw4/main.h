#include <stdio.h>
#include <locale.h>
#include <cstring>
#include <cmath>

#include <mpi.h>

// --- MPI --- //
#define ROOT_RANK 0
#define __ROOT if ((rank) == ROOT_RANK)

// --- Math --- //
const double a = 1.0e5;
const double EPS = 1.0e-14;

const double phi_0 = 0.0;

const int
  X1 = -1, X2 = 1,
  Y1 = -1, Y2 = 1,
  Z1 = -1, Z2 = 1;

const double
  Dx = X2 - X1,
  Dy = Y2 - Y1,
  Dz = Z2 - Z1;

const int
  Nx = 250,
  Ny = 250,
  Nz = 250;

const double
  hx = Dx / (Nx - 1),
  hy = Dy / (Ny - 1),
  hz = Dz / (Nz - 1);

const double B = 1 / (2/(hx*hx) + 2/(hy*hy) + 2/(hz*hz) + a);

int iter(int i, int j, int k) { return Ny*Nz*i + j*Nz + k; }

double max(double a, double b) { return a > b ? a : b; }
double min(double a, double b) { return a > b ? b : a; }
