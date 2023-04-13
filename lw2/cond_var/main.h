#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define err_exit(code, str) { \
    std::cerr << str << ": " << std::strerror(code) << std::endl; \
    exit(EXIT_FAILURE); \
}