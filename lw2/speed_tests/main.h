#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>

using namespace std;


#define ARGS_COUNT 1 + 1
#define LOOP_COUNT 10
#define OPERATION_COUNT 10


static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

#define err_exit(code, str) { \
    std::cerr << str << ": " << std::strerror(code) << std::endl; \
    exit(EXIT_FAILURE); \
}
