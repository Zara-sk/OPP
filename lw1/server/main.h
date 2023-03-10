#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <math.h>

#define ARGS_COUNT 1 + 1

#define PORT 8080
#define MEANWHILE_CLIENT_COUNT 32
#define RESPONSE_LENGTH 250

typedef struct ThreadArgs {
  size_t request_number;
  int client_fd;
} ThreadArgs;

using namespace std;


static const char* RESPONSE_TEMPLATE =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
  "<!DOCTYPE html>"
  "<html>"
    "<body>"
      "<h1>Request number %zu has been processed</h1>"
    "</body>"
  "</html>\r\n";

static const char* RESPONSE_TEMPLATE_PHP =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
  "<!DOCTYPE html>"
  "<html>"
    "<body>"
      "<h1>Request number %zu has been processed</h1>"
      "<h1>PHP version: %s</h1>"
    "</body>"
  "</html>\r\n";
  
static void throw_error(char *msg);

static char* get_php_version();

static int digits_count(int v) { return floor(log10(v) + 1); }

static void *request_handler(void* arg);

void start_server(size_t stack_size);