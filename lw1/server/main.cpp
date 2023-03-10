#include "main.h"

static void throw_error(char *msg) {
  cout << msg << endl;
  exit(EXIT_FAILURE);
}

static char* get_php_version() {
  char *php_version = (char*)malloc(sizeof(char) * 10);

  FILE* f = popen("php -r \"echo phpversion();\"", "r");

  if (fscanf(f, "%s", php_version) == 1) {
    return php_version;
  } else {
    cout << "Error while check php version" << endl;
    exit(EXIT_FAILURE);
  }
}


// функция для выполнения в потоке
static void *request_handler(void* arg) {
  ThreadArgs* args = (ThreadArgs*) arg;

  // char* php_version = get_php_version();

  char response[RESPONSE_LENGTH];
  snprintf(
    response, RESPONSE_LENGTH, RESPONSE_TEMPLATE_PHP, args->request_number, get_php_version()
  );

  write(args->client_fd, response, RESPONSE_LENGTH);
	shutdown(args->client_fd, SHUT_WR);

  cout << " + [" << args->request_number << "]" << endl;
  pthread_exit(NULL);

  delete args;
}

void start_server(size_t stack_size) {
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);
  int err = 0;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) throw_error("Error while open socket!");

  int opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(PORT);

  err = bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
  if (err < 0) cout << err;

  err = listen(sock, MEANWHILE_CLIENT_COUNT);
  if (err < 0) throw_error("Error while bind!");

  
  pthread_attr_t thread_attr;
  err = pthread_attr_init(&thread_attr);
  if (err != 0) throw_error("Error ocured while phtread attr init");

  err = pthread_attr_setstacksize(&thread_attr, stack_size * 1024);
  if (err != 0) throw_error("Error ocured while set stack size");

  size_t request_counter = 1;
  while(true) {
    int client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
    cout << "Got connection [" << request_counter << "]" << endl;

    if (client_fd == -1) {
      cout << "Can't accept" << endl;
      continue;
    }

    ThreadArgs* args = new ThreadArgs;
    args->request_number = request_counter; 
    args-> client_fd = client_fd;

    pthread_t thread;
    err = pthread_create(&thread, &thread_attr, request_handler, (void*) args);
    if (err < 0) throw_error("Error while create thread"); 
    
    request_counter++;
  }
  pthread_attr_destroy(&thread_attr);
}

int main(int argc, char* argv[]) {
  cout << get_php_version() << endl;
  // получаем аргументы из консоли
  if (argc != ARGS_COUNT) {
    cout << "Wrong arguments! Must be <stack_size>." << endl;
    exit(EXIT_FAILURE);
  }

  size_t stack_size = atoi(argv[1]);
  if (stack_size == 0) {
    cout << "Wrong argument type! Must be positive integer." << endl;
    exit(EXIT_FAILURE);
  }

  start_server(stack_size);

  return 0;
}