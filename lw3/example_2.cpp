#include <omp.h>
#include <iostream>

#define ARRAY_SIZE 100

using namespace std;

int main() {
  int sum = 0;

  int a[ARRAY_SIZE], id, size;

  for(int i = 0; i<ARRAY_SIZE; i++) {
    a[i] = i;
  }

  // Применение опции reduction
  #pragma omp parallel private(size, id)
  { // Начало параллельной области
    id = omp_get_thread_num();
    size = omp_get_num_threads();

    // Разделяем работу между потоками
    int integer_part = ARRAY_SIZE / size;
    int remainder = ARRAY_SIZE % size;
    int a_local_size = integer_part + ((id < remainder) ? 1 : 0);

    int start = integer_part * id + ((id < remainder) ? id : remainder);

    int end = start + a_local_size;
    
    // Каждый поток суммирует элементы
    // своей части массива
    int partial_sum = 0;
    for(int i = start; i < end; i++)
    {
      partial_sum += a[i];
    }

    #pragma omp critical(sum)
    sum += partial_sum;
    
    // Каждый поток выводит свою частичную сумму
    printf("Thread %d, partial_sum = %d\n", id, partial_sum);
  }
  // Благодаря опции reduction сумма частичных
  // результатов добавлена к значению переменной
  // sum начального потока
  cout << "Final sum = " << sum << endl;

  return 0;
}