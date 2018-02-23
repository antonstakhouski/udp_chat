#ifndef H_CLIENT_LIB
#define H_CLIENT_LIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>

#include <netdb.h>

#define BUF_SIZE 120
#define MAX_USERS 20

struct listener_data {
    struct ifaddrs my_addr;
    int port;
};

struct user_data {
    sa_family_t sin_family;
    in_addr_t sin_addr;
    char nick[20];
    time_t timestamp;
};

enum role {SENDER, RECEIVER};

#endif //H_CLIENT_LIB
