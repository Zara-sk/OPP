#include <omp.h>
#include <iostream>
#include <time.h>

#define OPERATIONS_COUNT 4.5e+8
#define ARRAY_SIZE 100

using namespace std;

static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

int f(int value) {
  double result;
  for (int i = 0; i < OPERATIONS_COUNT; i++) {
    result += value * 1.0 / (i + 1);
  }
  return (int)result;
}

int main() {
  int a[ARRAY_SIZE], b[ARRAY_SIZE];

  // Инициализация массива b
  for(int i = 0; i < ARRAY_SIZE; i++)
    b[i] = i;

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // Директива OpenMP для распараллеливания цикла
  #pragma omp parallel for
  for(int i = 0; i < ARRAY_SIZE; i++) {
    a[i] = f(b[i]);
    b[i] = 2*a[i];
  }
  int result = 0;

  clock_gettime(CLOCK_MONOTONIC, &end);
  cout << "time: " << timeDiff(start, end) << endl;

  // Далее значения a[i] и b[i] используются, например, так:
  #pragma omp parallel for reduction(+ : result)
  for(int i = 0; i < ARRAY_SIZE; i++){
    result += (a[i] + b[i]);
  }
  cout << "Result = " << result << endl;
  //
  return 0;
}