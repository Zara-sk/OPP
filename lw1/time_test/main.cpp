#include "main.h"

static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

void *thread_job(void *arg) {
  struct timespec start, end;
  ThreadArgs *args = (ThreadArgs*)arg;

  clock_gettime(CLOCK_MONOTONIC, &start);
  for (size_t i = 0; i < args->operations_count; i++) {
    float result = 22.325 * 1778.902;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  args->job_time = timeDiff(start, end);
  
  pthread_exit(NULL);
}

void *thread_job_attr(void *arg) {
  
  pthread_attr_t thread_attr;
  pthread_getattr_np(pthread_self(), &thread_attr);

  int detachstate;
  size_t guardsize;
  void *stackaddr;
  size_t stacksize;

  int err = pthread_attr_getdetachstate(&thread_attr, &detachstate);
  checkErr(err);
  err = pthread_attr_getguardsize(&thread_attr, &guardsize);
  checkErr(err);
  err = pthread_attr_getstack(&thread_attr, &stackaddr, &stacksize);
  checkErr(err);

  printf(
    "\ndetach state: %s\nguard size: %zu bytes\nstack size: %zu bytes\nstack address: %p\n",
    (detachstate == PTHREAD_CREATE_DETACHED) ? "PTHREAD_CREATE_DETACHED" : "PTHREAD_CREATE_JOINABLE",
    guardsize, stacksize, stackaddr
  );

  pthread_exit(NULL);
}

static void checkErr(int err) {
  if (err != 0) {
      cout << "An error ocured: " << strerror(err) << endl;
      exit(-1);
  }
}



// static void get_time_stat() {

//   struct timespec start, end;

//   pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * THREADS_COUNT);

//   pthread_attr_t thread_attr;
  
//   int err;

//   err = pthread_attr_init(&thread_attr);


//   clock_gettime(CLOCK_MONOTONIC, &start);
//   err = pthread_create(&threads[0], NULL, thread_job, NULL);
//   checkErr(err);
//   clock_gettime(CLOCK_MONOTONIC, &end);

//   double totalTime = timeDiff(start, end);

//   printf("time for create: \t%.10f s.\n", totalTime);

//   for (uint8_t i = 0; i < THREADS_COUNT; i++) {
//     err = pthread_join(threads[i], NULL);
//     checkErr(err);
//   }
// }

static ThreadTimeStat calc_thread_time(size_t operations_count) {
  struct timespec start, end;

  pthread_t thread;
  pthread_attr_t thread_attr;
  ThreadArgs thread_args = (ThreadArgs){operations_count, 0.0};

  int err;
  err = pthread_attr_init(&thread_attr);
  checkErr(err);

  err = pthread_attr_setstacksize(&thread_attr, 5*1024*1024);
  checkErr(err);

  clock_gettime(CLOCK_MONOTONIC, &start);
  err = pthread_create(&thread, &thread_attr, thread_job, (void*) &thread_args);
  clock_gettime(CLOCK_MONOTONIC, &end);
  checkErr(err);

  err = pthread_join(thread, NULL);
  pthread_attr_destroy(&thread_attr);
  checkErr(err);

  return (ThreadTimeStat){timeDiff(start, end), thread_args.job_time};
}

static size_t calc_average_operations_time(uint16_t operations_count_start, uint16_t operations_step) {

  double
    create_time = 0.0,
    job_time    = 0.0;

  size_t operations_count = operations_count_start;

  while (true) {
    
    for (size_t i = 0; i < LOOP_OPERATIONS_COUNT; i++) {
      ThreadTimeStat stat = calc_thread_time(operations_count);
      create_time += stat.create_time;
      job_time += stat.job_time;
    }

    create_time /= LOOP_OPERATIONS_COUNT;
    job_time    /= LOOP_OPERATIONS_COUNT;

    cout << "create " << create_time << endl;
    cout << "job " << job_time << endl;
    cout << endl;

    if (create_time < job_time) {
      break;
    } else {
      operations_count += operations_step;
    }
  }

  return operations_count;
}

int main(int argc, char* argv[]) {

  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments! Must be <operations_count_start> <operations_step>." << endl;
    exit(EXIT_FAILURE);
  }

  uint16_t operations_count_start = atoi(argv[1]),
           operations_step        = atoi(argv[2]);

  if (operations_count_start == 0 || operations_step == 0) {
    cout << "Wrong argument type! Must be positive integer." << endl;
    exit(EXIT_FAILURE);
  }

  size_t operations_count = calc_average_operations_time(operations_count_start, operations_step);
  
  cout << "Operations count: " << operations_count << endl;

  pthread_t thread;
  pthread_attr_t thread_attr;

  int err = pthread_attr_init(&thread_attr);
  checkErr(err);

  err = pthread_attr_setstacksize(&thread_attr, 5*1024*1024);
  checkErr(err);

  err = pthread_create(&thread, &thread_attr, thread_job_attr, NULL);
  checkErr(err);

  pthread_join(thread, NULL);
  pthread_attr_destroy(&thread_attr);

  return 0;
}