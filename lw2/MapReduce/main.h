#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <map>

using namespace std;

typedef void*(*function)(void*);

#define ARGS_COUNT 1 + 2
#define THREADS_COUNT 6
#define ARRAY_SIZE 100

static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

#define err_exit(code, str) { \
    std::cerr << str << ": " << std::strerror(code) << std::endl; \
    exit(EXIT_FAILURE); \
}

typedef struct Slice {
  vector<int>::iterator iter;
  int slice_size;
  map<int, int> *result;
} Slice;

typedef struct RSlice {
  map<int, vector<int>>::iterator iter;
  int slice_size;
  map<int, int> *result;
} RSlice;