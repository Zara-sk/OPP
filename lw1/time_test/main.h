#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <time.h>

// #define THREADS_COUNT 4
#define LOOP_OPERATIONS_COUNT 1e+2
#define ARGS_COUNT 2 + 1

using namespace std;

typedef struct ThreadTimeStat {
  double create_time;
  double job_time;
} ThreadTimeStat;

typedef struct ThreadArgs {
  size_t operations_count;
  double job_time;
} ThreadArgs;

static double timeDiff(timespec start, timespec stop);

static void checkErr(int err);

void* thread_job(void* arg);

static ThreadTimeStat calc_thread_time(size_t operations_count);

static size_t calc_average_operations_time(uint16_t operations_count_start, uint16_t operations_step);