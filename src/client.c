#include "client_lib.h"

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

void* listen_chat(void* arg)
{
    int sd;
    struct ifaddrs my_addr = *((struct ifaddrs*)arg);

    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating listening socket");
        exit(0);
    }

    int bcast = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (char *) &bcast, sizeof(bcast)))
        perror("Set Broadcast option");

    int portno = 12345;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = my_addr.ifa_addr->sa_family;
    serv_addr.sin_port = htons(portno);

    serv_addr.sin_addr.s_addr = (&((struct sockaddr_in*)my_addr.ifa_ifu.ifu_broadaddr)->sin_addr)->s_addr;

    assert(!bind(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)));

    puts("----Connected to the chat----");
    char buffer[BUF_SIZE];
    int n;
    struct sockaddr_in from;
    socklen_t fromlen;
    fromlen = sizeof(struct sockaddr_in);
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        if ((n = recvfrom(sd, buffer, BUF_SIZE - 1, 0,
            (struct sockaddr *)&from,  &fromlen)) > 0) {
            printf("%s", buffer);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <net_interface>\n", argv[0]);
        exit(0);
    }

    struct ifaddrs my_addr;
    if (get_int_info(&my_addr, argv[1]))
        return 0;

    show_int_info(&my_addr);

    pthread_t listener;
    pthread_create(&listener, NULL, &listen_chat, &my_addr);
    while(1);

    /*char nickname[20];*/

    /*printf("Enter you nickname(20 characters max): ");*/
    /*scanf("%s", nickname);*/
    /*printf("> ");*/

    return 0;
}
