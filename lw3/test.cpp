#include <omp.h>
#include <iostream>
#include <random>
#include <math.h>
using namespace std;
int main() {
// Установка числа потоков (4)
// Проявится при выполнении второй параллельной области
omp_set_num_threads(4);
// Исполнять код следующей области будут 2 потока,
// так как действие omp_set_num_threads()
// отменяется опцией num_threads
#pragma omp parallel num_threads(2)
{
int id = omp_get_thread_num();
if(0 == id) {
int size = omp_get_num_threads();
cout << "Number of threads = " << size << endl;
cout.flush();
}
}
int i, sum = 0, npoints = 10000000;
// В следующей части кода происходит приближенное вычисление
// числа Пи. В квадрате со стороной равной 2 выбрасывается
// npoints случайных точек, затем число Пи оценивается на
// основе соотношения числа точек, попавших во вписанный
// круг радиуса 1, к npoints.
// Показано, как разделить декларацию параллельной области
// на 2 строки с помощью обратного слэша.
// В этой области в команде будет 4 потока.
#pragma omp parallel private (i) shared (npoints) \
reduction (+: sum)
{
uniform_real_distribution<double> unif(-1.0,1.0);
random_device rd;
default_random_engine re(rd());
int numthreads = omp_get_num_threads();
for(i = 0; i < npoints / numthreads; i++)
{
double randx = unif(re);
double randy = unif(re);
if (sqrt(randx * randx + randy * randy) <= 1.0)
sum++;
}
}
double pi = 4.0*(double(sum)/npoints);
cout << "pi (approximately) = " << pi << endl;
return 0;
}