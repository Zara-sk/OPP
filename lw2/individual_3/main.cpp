#include "main.h"

pthread_mutex_t mutex;
pthread_cond_t cond;


enum store_state {EMPTY, FULL} state = EMPTY;
list<int> store;


// функция поставщика
void *producer(void *arg) {
  size_t* consumers_count = (size_t*)arg;
  int err;
  
  while(true) {
    // Захватываем мьютекс и ожидаем освобождения склада
    err = pthread_mutex_lock(&mutex);
    if(err != 0) err_exit(err, "Cannot lock mutex");

    while(state == FULL) {
      err = pthread_cond_wait(&cond, &mutex);
      if(err != 0) err_exit(err, "Cannot wait on condition variable");
    }

    // Получен сигнал, что на складе не осталось товаров.
    // Производим новый товар.
    store.push_back(rand());
    cout << "Created number " << store.back() << endl;

    if (store.size() > 0) {
      state = FULL;
    }
    
    // Посылаем сигнал, что на складе появился товар.
    err = pthread_cond_signal(&cond);
    if(err != 0) err_exit(err, "Cannot send signal");

    err = pthread_mutex_unlock(&mutex);
    if(err != 0) err_exit(err, "Cannot unlock mutex");
  }
}

// функция потребителя
void *consumer(void *arg) {
  int err;
  size_t* consumers_count = (size_t*)arg;

  while(true) {
    // Захватываем мьютекс и ожидаем появления товаров на складе
    err = pthread_mutex_lock(&mutex);
    if(err != 0) err_exit(err, "Cannot lock mutex");

    while(state == EMPTY) {
      err = pthread_cond_wait(&cond, &mutex);
      if(err != 0) err_exit(err, "Cannot wait on condition variable");
    }

    // Получен сигнал, что на складе имеется товар.
    // Потребляем его.
    int st = store.front();
    store.pop_front();

    if (store.size() == 0) {
      state = EMPTY;
    }
    
    // Посылаем сигнал, что на складе не осталось товаров.
    err = pthread_cond_signal(&cond);
    if(err != 0) err_exit(err, "Cannot send signal");

    err = pthread_mutex_unlock(&mutex);
    if(err != 0) err_exit(err, "Cannot unlock mutex");

    sleep(1);
    cout << "Consumed number " << st << endl;

  }
}
int main(int argc, char* argv[]) {
  int err;

  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments! Must be <consumers_count>." << endl;
    exit(EXIT_FAILURE);
  }
  size_t consumers_count = atoi(argv[1]);

  if (consumers_count == 0) {
    cout << "Wrong argument type! Must be positive integer." << endl;
    exit(EXIT_FAILURE);
  }

  pthread_t thread_producer;
  pthread_t *threads_consumers = (pthread_t*)malloc(sizeof(pthread_t) * consumers_count);

  // Инициализируем мьютекс и условную переменную
  err = pthread_cond_init(&cond, NULL);
  if(err != 0) err_exit(err, "Cannot initialize condition variable");

  err = pthread_mutex_init(&mutex, NULL);
  if(err != 0) err_exit(err, "Cannot initialize mutex");

  // Создаём потоки
  err = pthread_create(&thread_producer, NULL, producer, (void*)&consumers_count);
  if(err != 0) err_exit(err, "Cannot create thread 1");

  for (int i = 0; i < consumers_count; i++) {
    err = pthread_create(&threads_consumers[i], NULL, consumer, (void*)&consumers_count);
    if(err != 0) err_exit(err, "Cannot create thread 2");
  }

  pthread_join(thread_producer, NULL);
  for (int i = 0; i < consumers_count; i++) {
    err = pthread_join(threads_consumers[i], NULL);
    if(err != 0) err_exit(err, "Cannot join thread 2");
  }

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}