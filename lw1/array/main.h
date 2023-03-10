#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

#define ARGS_COUNT 3 + 1
#define PRINT_ARRAYS 1


using namespace std;

typedef double (*routine)(double);

// Структура для обработки функцией потока
typedef struct Slice {
  double *array;
  size_t slice_size;
  routine function;
} Slice;

static double timeDiff(timespec start, timespec stop);

static void checkErr(int err);

// Функции над элементами массива
static double sqr(double value) { return value * value; }
static double inc(double value) { return ++value; }
static double inv(double value) { return 1 / value; }

static routine dispatch_function(size_t function_number);

static void print_array(double *array, int array_size);

static void* apply_to_slice(void* arg);

static void apply(double *array, size_t array_size, routine function, uint16_t threads_count);