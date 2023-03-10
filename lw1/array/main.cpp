#include "main.h"

static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

static void checkErr(int err) {
  if (err != 0) {
      cout << "An error ocured: " << strerror(err) << endl;
      exit(-1);
  }
}


// определяет по номеру какую функцию выполнять над массивом
static routine dispatch_function(size_t function_number) {
  switch (function_number) {
    case 1:
      return sqr;
    case 2:
      return inc;
    case 3:
      return inv;
    default:
      cout << "Wrong function number!\n1 - sqr\n2 - inrcrement\n3 - inverse" << endl;
      exit(EXIT_FAILURE);
  }
}

static void print_array(double *array, int array_size) {
  cout << "[";
  for (size_t i = 0; i < array_size; i++) {
    cout << array[i];
    if (array_size - i > 1) {
      cout << ", ";
    }
  }
  cout << "]" << endl;
}

// Функция потока
static void* apply_to_slice(void* arg) {
  Slice* slice = (Slice*)arg;
  for (size_t i = 0; i < slice->slice_size; i++) {
    slice->array[i] = slice->function(slice->array[i]);
  }
  pthread_exit(NULL);
}

static void apply(double *array, size_t array_size, routine function, uint16_t threads_count) {
  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);

  // Определяем массив блоков для каждого потока
  Slice* slices = (Slice*)malloc(sizeof(Slice) * threads_count);

  size_t slice_size = array_size / threads_count; // число элементов, которое будет обрабатывать каждый поток
  uint8_t remainder = slice_size % threads_count; // остаток, который распределяется по первым потокам

  int err;
  size_t shift = 0;
  
  // Распределяем работу по потокам
  for (uint8_t i = 0; i < threads_count; i++) {
    // создаем параметры для потока
    slices[i] = (Slice){
      array + shift,
      slice_size + (i < remainder ? 1 : 0), // если один из первых - дополняем из остатка
      function
    };
    shift += slices[i].slice_size;

    err = pthread_create(&threads[i], NULL, apply_to_slice, (void*)&slices[i]);
    checkErr(err);
  }

  // ожидаем завершение всех потоков
  for (uint8_t i = 0; i < threads_count; i++) {
    err = pthread_join(threads[i], NULL);
    checkErr(err);
  }

  free(threads);
  free(slices);
}

int main(int argc, char* argv[]) {

  // получаем аргументы из консоли
  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments! Must be <array_size> <threads_count> <function_number>." << endl;
    exit(EXIT_FAILURE);
  }

  size_t array_size      = atoi(argv[1]),
         threads_count   = atoi(argv[2]),
         function_number = atoi(argv[3]);

  if (array_size == 0 || function_number == 0 || threads_count == 0) {
    cout << "Wrong argument type! Must be positive integer." << endl;
    exit(EXIT_FAILURE);
  }

  // определяем какую функцию выполнять
  routine function = dispatch_function(function_number);

  // создаем массив
  double *array = (double*)malloc(sizeof(double) * array_size);

  // Инициализируем массив случайными значениями от 0 до 1000
  for (size_t i = 0; i < array_size; i++) {
    array[i] = rand() % 1000;
  }

  if (PRINT_ARRAYS) print_array(array, array_size);

  apply(array, array_size, function, threads_count);

  if (PRINT_ARRAYS) print_array(array, array_size);

  return 0;
}