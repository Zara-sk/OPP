#include "main.h"



enum store_state {EMPTY, FULL} state = EMPTY;
int store;


typedef bool my_cond_t;

int my_cond_wait(my_cond_t *cond, pthread_mutex_t *mutex) {
  pthread_mutex_unlock(mutex);

  while (true) {
    if (*cond == true) {break;}
  }

  *cond = false;
  int err = pthread_mutex_lock(mutex);
  if (err != 0) return err;
  return 0;
}

int my_cond_signal(my_cond_t *cond) {
  *cond = true;
  return 0;
}


pthread_mutex_t mutex;
my_cond_t cond;

// функция поставщика
void *producer(void *arg) {
  int err;
  
  while(true) {
    // Захватываем мьютекс и ожидаем освобождения склада
    err = pthread_mutex_lock(&mutex);
    if(err != 0) err_exit(err, "Cannot lock mutex");

    while(state == FULL) {
      err = my_cond_wait(&cond, &mutex);
      if(err != 0) err_exit(err, "Cannot wait on condition variable");
    }

    // Получен сигнал, что на складе не осталось товаров.
    // Производим новый товар.
    store = rand();
    state = FULL;
    
    // Посылаем сигнал, что на складе появился товар.
    err = my_cond_signal(&cond);
    if(err != 0) err_exit(err, "Cannot send signal");

    err = pthread_mutex_unlock(&mutex);
    if(err != 0) err_exit(err, "Cannot unlock mutex");
  }
}

// функция потребителя
void *consumer(void *arg) {
  int err;

  while(true) {
    // Захватываем мьютекс и ожидаем появления товаров на складе
    err = pthread_mutex_lock(&mutex);
    if(err != 0) err_exit(err, "Cannot lock mutex");

    while(state == EMPTY) {
      err = my_cond_wait(&cond, &mutex);
      if(err != 0) err_exit(err, "Cannot wait on condition variable");
    }

    // Получен сигнал, что на складе имеется товар.
    // Потребляем его.
    cout << "Consuming number " << store << "..." << flush;
    sleep(1);
    
    cout << " \tdone" << endl;
    state = EMPTY;
    
    // Посылаем сигнал, что на складе не осталось товаров.
    err = my_cond_signal(&cond);
    if(err != 0) err_exit(err, "Cannot send signal");

    err = pthread_mutex_unlock(&mutex);
    if(err != 0) err_exit(err, "Cannot unlock mutex");
  }
}
int main() {
  int err;

  pthread_t thread_producer, thread_consumer;

  // Инициализируем мьютекс и условную переменную
  // err = pthread_cond_init(&cond, NULL);
  // if(err != 0) err_exit(err, "Cannot initialize condition variable");

  err = pthread_mutex_init(&mutex, NULL);
  if(err != 0) err_exit(err, "Cannot initialize mutex");

  // Создаём потоки
  err = pthread_create(&thread_producer, NULL, producer, NULL);
  if(err != 0) err_exit(err, "Cannot create thread 1");

  err = pthread_create(&thread_consumer, NULL, consumer, NULL);
  if(err != 0) err_exit(err, "Cannot create thread 2");

  pthread_join(thread_producer, NULL);
  pthread_join(thread_consumer, NULL);

  pthread_mutex_destroy(&mutex);
  // pthread_cond_destroy(&cond);
}