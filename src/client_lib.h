#ifndef H_CLIENT_LIB
#define H_CLIENT_LIB

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <ifaddrs.h>

#include <netdb.h>

#define BUF_SIZE 120
#define MAX_USERS 20

#define CHAT_GROUP "239.0.0.1"

enum role {SENDER, RECEIVER};
enum scope {BROADCAST_MODE, MULTICAST_MODE};

struct listener_data {
    struct ifaddrs my_addr;
    int port;
    enum scope my_scope;
};

struct user_data {
    sa_family_t sin_family;
    in_addr_t sin_addr;
    char nick[20];
    time_t timestamp;
};

#endif //H_CLIENT_LIB
