#include "main.h"

pthread_spinlock_t spinlock;
pthread_mutex_t    mutex;


void *thread_job_mutex(void *arg) {
  int err = 0;
  
  err = pthread_mutex_lock(&mutex);
  if(err != 0) err_exit(err, "Cannot lock mutex");

  for (size_t i = 0; i < OPERATION_COUNT; i++) {
    float result = 22.325 * 1778.902;
  }
  
  err = pthread_mutex_unlock(&mutex);
  if(err != 0) err_exit(err, "Cannot unlock mutex");
  
  pthread_exit(NULL);
}

void *thread_job_spinlock(void *arg) {
  int err = 0;
  
  err = pthread_spin_lock(&spinlock);
  if(err != 0) err_exit(err, "Cannot lock mutex");

  for (size_t i = 0; i < OPERATION_COUNT; i++) {
    float result = 22.325 * 1778.902;
  }
  
  err = pthread_spin_unlock(&spinlock);
  if(err != 0) err_exit(err, "Cannot unlock mutex");
  
  pthread_exit(NULL);
}

double calculate_mutex_time(int threads_count) {

  double time_summ = 0;
  struct timespec start, end;
  int err = 0;

  for (size_t i = 0; i < LOOP_COUNT; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);

    for (size_t j = 0; j < threads_count; j++) {
      err = pthread_create(&threads[j], NULL, thread_job_mutex, NULL);
      if (err != 0) err_exit(err, "Can't create thread");
    }
    
    for (size_t j = 0; j < threads_count; j++) {
      err = pthread_join(threads[j], NULL);
      if (err != 0) err_exit(err, "Can't create thread");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time_summ += timeDiff(start, end);
  }

  return time_summ / LOOP_COUNT;

}
double calculate_spinlock_time(int threads_count) {

  double time_summ = 0;
  struct timespec start, end;
  int err = 0;

  for (size_t i = 0; i < LOOP_COUNT; i++) {

    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);

    for (size_t j = 0; j < threads_count; j++) {
      err = pthread_create(&threads[j], NULL, thread_job_spinlock, NULL);
      if (err != 0) err_exit(err, "Can't create thread");
    }
    
    for (size_t j = 0; j < threads_count; j++) {
      err = pthread_join(threads[j], NULL);
      if (err != 0) err_exit(err, "Can't create thread");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time_summ += timeDiff(start, end);

    free(threads);
  }

  return time_summ / LOOP_COUNT;
}


void calculate_primitive_time(int threads_count) {

  double mutex_time = calculate_mutex_time(threads_count);
  double spinlock_time = calculate_spinlock_time(threads_count);

  cout << "mutex:    " << mutex_time << " ms." << endl;
  cout << "spinlock: " << spinlock_time << " ms." << endl;

}

int main(int argc, char* argv[]) {
  int err; // Код ошибки

  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments" << endl;
    exit(EXIT_FAILURE);
  }

  int threads_count = atoi(argv[1]);
  if (threads_count < 1) {
    cout << "Wrong threads count" << endl;
    exit(EXIT_FAILURE);
  }

  err = pthread_mutex_init(&mutex, NULL);
  if(err != 0) err_exit(err, "Cannot initialize mutex");

  err = pthread_spin_init(&spinlock, NULL);
  if(err != 0) err_exit(err, "Cannot initialize mutex");


  calculate_primitive_time(threads_count);

  
  pthread_mutex_destroy(&mutex);
  pthread_spin_destroy(&spinlock);
}