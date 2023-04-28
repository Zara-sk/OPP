#include "main.h"


void init_graph(Graph *graph, int nodes_count, double fullness) {
  graph->matrix = (Matrix)malloc(sizeof(int*) * nodes_count);
  for (int i = 0; i < nodes_count; i++) {
    graph->matrix[i] = (int*)malloc(sizeof(int) * nodes_count);
    for (int j = 0; j < nodes_count; j++) {
      graph->matrix[i][j] = 0;
    }
    graph->nodes.push_back((double)(rand() % 4096 + 0.1*(rand() % 10)));
  }

  vector<int> unallocated;
  vector<int> allocated;
  for (int i = 0; i < nodes_count; i++) unallocated.push_back(i);

  // Создаем связанное дерево
  int new_node, idx;
  for (int i = 0; i < nodes_count; i++) {
    idx = rand() % (nodes_count - allocated.size());
    new_node = unallocated[idx];
    unallocated.erase(unallocated.begin() + idx);

    if (i > 0) {
      idx = rand() % allocated.size();
      graph->matrix[new_node][allocated[idx]] = 1;
      graph->matrix[allocated[idx]][new_node] = 1;
    }
    allocated.push_back(new_node);
  }

    // Дополняем ребра
  for (int i = 0; i < nodes_count - 1; i++) {
    for (int j = i + 1; j < nodes_count; j++) {
      if (rand() % 100 < fullness * 100) {
        graph->matrix[i][j] = 1;
        graph->matrix[j][i] = 1;
      }
    }
  }
}


void print_graph(Graph graph, int nodes_count) {
  for (int i = 0; i < nodes_count; i++) {
    for (int j = 0; j < nodes_count; j++) {
      cout << graph.matrix[i][j] << " ";
    }
    cout << endl;
  }
}

void test(double a) {
  double res = 0;
  for (int i = 0; i < 100000; i++) {
    res += (a - i) * a;
  }
}

void dfs_single_thread(Graph graph, int nodes_count, function func) {
  vector<int> color;
  for (int i = 0; i < nodes_count; i++) color.push_back(0);

  vector<int> queue;
  queue.push_back(0);
  color[0] = 1;

  MPI_Request nullReq;
  int next_rank = 1;

  int current_node;
  while (queue.size()) {
    current_node = queue[0];
    queue.erase(queue.begin());

    func(graph.nodes[current_node]);

    // соседи
    for (int i = 0; i < nodes_count; i++) {
      if (graph.matrix[current_node][i] == 1 && color[i] == 0) {
        queue.insert(queue.begin(), 1, i);
        color[i] = 1;
      }
    }
  }
}

double dfs(Graph graph, int nodes_count, function func) {
  int rank, processes_number;

  MPI_Comm_size(MPI_COMM_WORLD, &processes_number); // получаем количество запущенных процессов
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // получаем id процесса

  __ROOT {
    double start = MPI_Wtime();

    vector<int> color;
    for (int i = 0; i < nodes_count; i++) color.push_back(0);

    vector<int> queue;
    queue.push_back(0);
    color[0] = 1;

    MPI_Request nullReq;
    int next_rank = 1;

    int current_node;
    while (queue.size()) {
      current_node = queue[0];
      queue.erase(queue.begin());

      MPI_Isend(&graph.nodes[current_node], 1, MPI_DOUBLE, next_rank, 0, MPI_COMM_WORLD, &nullReq);
      next_rank++;
      if (next_rank == processes_number) next_rank = 1;

      // соседи
      for (int i = 0; i < nodes_count; i++) {
        if (graph.matrix[current_node][i] == 1 && color[i] == 0) {
          queue.insert(queue.begin(), 1, i);
          color[i] = 1;
        }
      }
    }
    for (int i = 1; i < processes_number; i++) {
      double exitData;
      MPI_Isend(&exitData, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &nullReq);
    }
    double end = MPI_Wtime();
    return end - start;

  } else {
    double data, nullData;
    MPI_Status status;
    MPI_Request exitReq, dataReq;
    MPI_Irecv(&nullData, 1, MPI_DOUBLE, ROOT_RANK, 1, MPI_COMM_WORLD, &exitReq);
    MPI_Irecv(&data, 1, MPI_DOUBLE, ROOT_RANK, 0, MPI_COMM_WORLD, &dataReq);
    int exitFlag, dataFlag;

    while (true) {
      MPI_Test(&dataReq, &dataFlag, &status);
      if (dataFlag) {
        func(data);
        MPI_Irecv(&data, 1, MPI_DOUBLE, ROOT_RANK, 0, MPI_COMM_WORLD, &dataReq);
      } else {
        MPI_Test(&exitReq, &exitFlag, &status);
        if (exitFlag) {
          break;
        }
      }
    }
    return 0.0;
  }
}


int main(int argc, char* argv[]) {
  
  srand(time(NULL));
  int rank, processes_number;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // получаем id процесса
  MPI_Comm_size(MPI_COMM_WORLD, &processes_number); // получаем количество запущенных процессов

  int nodes_count = 5000;

  Graph graph;

  __ROOT
  init_graph(&graph, nodes_count, 0.5);

  double summ_time = 0;
  double summ_root_time = 0;

  for (int i = 0; i < 25; i++) {
    MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем начало
    double start = MPI_Wtime();

    if (processes_number == 1) {
      dfs_single_thread(graph, nodes_count, test);
    } else {
      summ_root_time += dfs(graph, nodes_count, test);
    }

    MPI_Barrier(MPI_COMM_WORLD); // Синхронизируем завершение работы
    double end = MPI_Wtime();

    summ_time += end - start;
  }

  MPI_Finalize();

  __ROOT
  {
    printf("time = %f\n", summ_time / 25);
    printf("root time = %f\n", summ_root_time / 25);
  }

  return 0;
}