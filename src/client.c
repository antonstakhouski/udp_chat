#include "client_lib.h"

char nickname[20];

pthread_mutex_t lock;
struct user_data users[MAX_USERS];

void show_int_info(const struct ifaddrs* tempIfAddr)
{
    void *tempAddrPtr = NULL;
    char addressOutputBuffer[INET_ADDRSTRLEN];

    if(tempIfAddr->ifa_addr->sa_family == AF_INET)
        tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_addr)->sin_addr;

    printf("Internet Address:  %s \n",
            inet_ntop(tempIfAddr->ifa_addr->sa_family,
                tempAddrPtr,
                addressOutputBuffer,
                sizeof(addressOutputBuffer)));

    printf("LineDescription :  %s \n", tempIfAddr->ifa_name);
    if(tempIfAddr->ifa_netmask != NULL)
    {
        if(tempIfAddr->ifa_netmask->sa_family == AF_INET)
            tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_netmask)->sin_addr;

        printf("Netmask         :  %s \n",
                inet_ntop(tempIfAddr->ifa_netmask->sa_family,
                    tempAddrPtr,
                    addressOutputBuffer,
                    sizeof(addressOutputBuffer)));
    }

    if(tempIfAddr->ifa_ifu.ifu_broadaddr != NULL)
    {
        printf("Broadcast Addr  :  ");
        if(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family == AF_INET)
            tempAddrPtr = &((struct sockaddr_in *)tempIfAddr->ifa_ifu.ifu_broadaddr)->sin_addr;

        printf("%s \n",
                inet_ntop(tempIfAddr->ifa_ifu.ifu_broadaddr->sa_family,
                    tempAddrPtr,
                    addressOutputBuffer,
                    sizeof(addressOutputBuffer)));
    }
    printf("\n");
}

int get_int_info(struct ifaddrs* my_addr, char* int_name)
{
    struct ifaddrs *interfaceArray = NULL, *tempIfAddr = NULL;
    int rc = 0;

    rc = getifaddrs(&interfaceArray);  /* retrieve the current interfaces */
    if (rc == 0)
    {
        for(tempIfAddr = interfaceArray; tempIfAddr != NULL; tempIfAddr = tempIfAddr->ifa_next)
        {
            if (!strcmp(int_name, tempIfAddr->ifa_name) && tempIfAddr->ifa_addr->sa_family == AF_INET)
                break;
        }

        if (tempIfAddr == NULL) {
            printf("Interface %s was not found\n", int_name);
            return 1;
        }

        *my_addr = *tempIfAddr;

        freeifaddrs(interfaceArray);             /* free the dynamic memory */
        interfaceArray = NULL;                   /* prevent use after free  */
    }
    else
    {
        printf("getifaddrs() failed with errno =  %d %s \n",
                errno, strerror(errno));
    }
    return rc;
}

int init_socket(int* sd, const struct listener_data* data, enum role my_role)
{
    if ((*sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating listening socket");
        exit(0);
    }

    int bcast = 1, reusep = 1, reusea = 1;
    if (setsockopt(*sd, SOL_SOCKET, SO_BROADCAST, (char *)&bcast, sizeof(bcast)))
        perror("Set socket options");

    if (setsockopt(*sd, SOL_SOCKET, SO_REUSEPORT, (char *)&reusep, sizeof(reusep)))
        perror("Set socket options");

    if (setsockopt(*sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reusea, sizeof(reusea)))
        perror("Set socket options");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_port = htons(data->port);

    if (my_role == RECEIVER) {
        serv_addr.sin_family = data->my_addr.ifa_ifu.ifu_broadaddr->sa_family;
        serv_addr.sin_addr.s_addr =
            (&((struct sockaddr_in*)data->my_addr.ifa_ifu.ifu_broadaddr)->sin_addr)->s_addr;
    } else {
        serv_addr.sin_family = data->my_addr.ifa_addr->sa_family;
        serv_addr.sin_addr.s_addr =
            (&((struct sockaddr_in*)data->my_addr.ifa_addr)->sin_addr)->s_addr;
    }

    assert(!bind(*sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)));
    return 0;
}

void show_users()
{
    puts("Users online:");
    int cnt = 1;
    time_t currtime = time(NULL);
    char addressOutputBuffer[INET_ADDRSTRLEN];
    struct tm* tm_info;
    char buffer[26];
    for(int i = 0; i < MAX_USERS; i++) {
        if (users[i].sin_addr && currtime - users[i].timestamp <= 60) {
            tm_info = localtime(&users[i].timestamp);
            strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
            printf("%d. %s on %s. Last seen on %s\n", cnt, users[i].nick,
                    inet_ntop(users[i].sin_family, &users[i].sin_addr,
                        addressOutputBuffer, sizeof(struct sockaddr_in)),
                    buffer);
            cnt++;
        }
    }
}

void* listen_chat(void* arg)
{
    struct listener_data data = *((struct listener_data*)arg);
    int listening_socket, sending_socket;
    init_socket(&listening_socket, &data, RECEIVER);
    init_socket(&sending_socket, &data, SENDER);

    struct sockaddr_in target;
    target.sin_family = data.my_addr.ifa_ifu.ifu_broadaddr->sa_family;
    target.sin_port = htons(data.port);
    target.sin_addr.s_addr =
        (&((struct sockaddr_in*)data.my_addr.ifa_ifu.ifu_broadaddr)->sin_addr)->s_addr;
    socklen_t targetlen = sizeof(struct sockaddr_in);

    pthread_mutex_lock(&lock);
    puts("----Connected to the chat----");
    pthread_mutex_unlock(&lock);
    char buffer[BUF_SIZE];
    char pong_msg[30];
    memset(pong_msg, 0, sizeof(pong_msg));
    sprintf(pong_msg, "%s: PONG!\n", nickname);
    int n;
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(struct sockaddr_in);
    char* msgptr;
    char nick[20];

    int found;
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        if ((n = recvfrom(listening_socket, buffer, BUF_SIZE - 1, 0, (struct sockaddr *)&from,  &fromlen)) > 0) {
            msgptr = strstr(buffer, ":");
            found = 0;
            if (!strcmp(msgptr + 2, "PING!\n") || !strcmp(msgptr + 2, "PONG!\n")) {
                memset(nick, 0, sizeof(nick));
                strncpy(nick, buffer, msgptr - buffer);

                for(int i = 0; i < MAX_USERS; i++) {
                    if (users[i].sin_addr == from.sin_addr.s_addr) {
                        strcpy(users[i].nick, nick);
                        users[i].timestamp = time(NULL);
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    time_t currtime = time(NULL);
                    for(int i = 0; i < MAX_USERS; i++) {
                        if (users[i].sin_addr == 0 || currtime - users[i].timestamp > 60) {
                            users[i].sin_family = from.sin_family;
                            users[i].sin_addr = from.sin_addr.s_addr;
                            strcpy(users[i].nick, nick);
                            users[i].timestamp = currtime;
                            found = 1;
                            break;
                        }
                    }
                }

                if (!strcmp(msgptr + 2, "PING!\n")) {
                    if ((n = sendto(sending_socket, pong_msg, strlen(pong_msg), 0, (struct sockaddr *)&target,  targetlen)) < 0) {
                        perror("Error sending a message");
                    }
                }

                continue;
            }
            pthread_mutex_lock(&lock);
            printf("%s", buffer);
            pthread_mutex_unlock(&lock);
        }
    }

    return NULL;
}

void* do_ping(void* arg)
{
    struct listener_data data = *((struct listener_data*)arg);
    int sd;
    init_socket(&sd, &data, SENDER);

    int n;
    struct sockaddr_in target;
    target.sin_family = data.my_addr.ifa_ifu.ifu_broadaddr->sa_family;
    target.sin_port = htons(data.port);
    target.sin_addr.s_addr =
        (&((struct sockaddr_in*)data.my_addr.ifa_ifu.ifu_broadaddr)->sin_addr)->s_addr;
    socklen_t targetlen = sizeof(struct sockaddr_in);

    char msg[30];
    memset(msg, 0, sizeof(msg));
    sprintf(msg, "%s: PING!\n", nickname);
    while(1) {
        if ((n = sendto(sd, msg, strlen(msg), 0, (struct sockaddr *)&target,  targetlen)) < 0) {
            perror("Error sending a message");
        }
        sleep(30);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: %s <net_interface> port\n", argv[0]);
        exit(0);
    }

     assert(!pthread_mutex_init(&lock, NULL));

    struct ifaddrs my_addr;
    if (get_int_info(&my_addr, argv[1]))
        return 0;

    show_int_info(&my_addr);
    int portno = atoi(argv[2]);

    struct listener_data data;
    data.my_addr = my_addr;
    data.port = portno;

    printf("Enter you nickname(20 characters max): ");
    scanf("%s", nickname);

    memset(users, 0, sizeof(users));
    pthread_t listener, pinger;
    pthread_create(&listener, NULL, &listen_chat, &data);
    pthread_create(&pinger, NULL, &do_ping, &data);

    char msg[80];
    char quote[120];
    int sd;
    init_socket(&sd, &data, SENDER);
    int n;
    struct sockaddr_in target;
    target.sin_family = my_addr.ifa_ifu.ifu_broadaddr->sa_family;
    target.sin_port = htons(portno);
    target.sin_addr.s_addr =
        (&((struct sockaddr_in*)my_addr.ifa_ifu.ifu_broadaddr)->sin_addr)->s_addr;
    socklen_t targetlen = sizeof(struct sockaddr_in);

    while(1) {
        scanf("%s", msg);
        if (!strcmp(msg, "/users")) {
            show_users();
            continue;
        }
        memset(quote, 0, sizeof(quote));
        sprintf(quote, "%s: %s\n", nickname, msg);
        if ((n = sendto(sd, quote, strlen(quote), 0, (struct sockaddr *)&target,  targetlen)) < 0) {
            perror("Error sending a message");
        }
    }

    pthread_mutex_destroy(&lock);

    return 0;
}
