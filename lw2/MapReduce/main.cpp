#include "main.h"

void* mapJob(void *arg) {
  int err;
  map<int, int> *result = new map<int, int>;

  Slice* slice = (Slice*)arg;

  for (auto i = slice->iter; i < slice->iter + slice->slice_size; i++) {
    if (result->find(*i) != result->end()) {
      result->at(*i) = result->at(*i) + 1;
    } else {
      result->insert(pair<int, int>(*i, 1));
    }
  }

  slice->result = result;
  pthread_exit(NULL);
}

void* reduceJob(void *arg) {
  int err;
  map<int, int> *result = new map<int, int>;

  RSlice* rslice = (RSlice*)arg;

  for (int i = 0; i < rslice->slice_size; i++) {
    int summ = 0;
    
    vector<int> *vec = new vector<int>;
    vec = &rslice->iter->second;

    for (vector<int>::iterator v = vec->begin(); v != vec->end(); v++) {
      summ += *v;
    }
    result->insert(pair<int, int>(rslice->iter->first, summ));

    rslice->iter++;
  }

  rslice->result = result;
  pthread_exit(NULL);
}


vector<int> MapReduce(vector<int> vec, function mapF, function reduceF, size_t threads_count) {
  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);
  Slice* slices = (Slice*)malloc(sizeof(Slice) * threads_count);
  RSlice* rslices = (RSlice*)malloc(sizeof(RSlice) * threads_count);

  int err;


  // ============= Map ============== //
  size_t slice_size = vec.size() / threads_count;
  size_t remainder = vec.size() % threads_count;
  size_t shift = 0;

  for (size_t i = 0; i < threads_count; i++) {
    map<int, int> *result = new map<int, int>;

    slices[i] = (Slice){
      vec.begin() + shift,
      slice_size + (i < remainder ? 1 : 0),
      result
    };
    shift += slices[i].slice_size;

    err = pthread_create(&threads[i], NULL, mapF, (void*)&slices[i]);
    if (err != 0) err_exit(err, "Can't create thread");
  }

  for (size_t i = 0; i < threads_count; i++) {
    err = pthread_join(threads[i], NULL);
    if (err != 0) err_exit(err, "Can't join thread");
  }
  delete[] threads;
  delete[] slices;


  // ============ Shaffle ============== //
  map<int, vector<int>> shaffle;
  
  for (size_t i = 0; i < threads_count; i++) {

    for (map<int, int>::iterator it = (*slices[i].result).begin(); it != (*slices[i].result).end(); it++) {
      if (shaffle.find(it->first) != shaffle.end()) {
        vector<int> new_vec = shaffle[it->first];
        new_vec.push_back(it->second);
        shaffle[it->first] = new_vec;
      } else {
        shaffle.insert(pair<int, vector<int>>(it->first, vector<int>{it->second}));
      }
    }
  }


  // ============ Reduce ============== //
  slice_size = shaffle.size() / threads_count;
  remainder = shaffle.size() % threads_count;

  auto it = shaffle.begin();

  for (size_t i = 0; i < threads_count; i++) {
    map<int, int> *result = new map<int, int>;

    rslices[i] = (RSlice){
      it,
      slice_size + (i < remainder ? 1 : 0),
      result
    };
    for (size_t j = 0; j < rslices[i].slice_size; j++) it++;

    err = pthread_create(&threads[i], NULL, reduceF, (void*)&rslices[i]);
    if (err != 0) err_exit(err, "Can't create thread");
  }

  for (size_t i = 0; i < threads_count; i++) {
    err = pthread_join(threads[i], NULL);
    if (err != 0) err_exit(err, "Can't join thread");
  }
  delete[] threads;


  // ============ Compare ============== //
  vector<int> result;
  for (size_t i = 0; i < threads_count; i++) {
    for (auto &itt: *rslices[i].result) {
      result.push_back((&itt)->second);
    }
  }

  delete[] rslices;
  return result;
}

int main(int argc, char* argv[]) {
  int err;

  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments! Must be <array_size> <threads_count>." << endl;
    exit(EXIT_FAILURE);
  }

  size_t array_size    = atoi(argv[1]),
         threads_count = atoi(argv[2]);

  if (array_size == 0 || threads_count == 0) {
    cout << "Wrong argument type! Must be positive integer." << endl;
    exit(EXIT_FAILURE);
  }

  vector<int> data;
  for (size_t i = 0; i < array_size; i++) {
    data.push_back(rand() % 10);
  }

  vector<int> result = MapReduce(data, mapJob, reduceJob, threads_count);

  for (int i = 0; i < 10 ; i++) {
    cout << i << ": " << result[i] << endl;
  }
}