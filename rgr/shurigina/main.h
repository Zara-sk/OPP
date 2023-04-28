#include <stdio.h>
#include <iostream>
#include <locale.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <unistd.h>

#include <mpi.h>

using namespace std;

// ---- MPI ---- //
#define ROOT_RANK 0
#define __ROOT if ((rank) == ROOT_RANK)


// --- Graph --- //
typedef int** Matrix;

// typedef struct Node {
//   int value_1;
//   double velue_2;
// } Node;

struct Graph {
  Matrix matrix;
  vector<double> nodes;
};


typedef void (*function)(double);
