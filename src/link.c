#include "link.h"
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include "util.h"


struct link {
    LinkType type;
    int (*receiveFunction)(Link *, uint8_t *);
    int (*sendFunction)(Link *, uint8_t *, int);

    int port;
    int socket;
    struct sockaddr_in sourceAddress;

    int clientSocket;
    struct sockaddr_in clientAddress;
    unsigned int clientLen;

    char *hostAddress;
};

static int
_udp_receive(Link *link, uint8_t *buffer)
{
    int numBytesReceived = recvfrom(link->socket, buffer, MTU, 0, (struct sockaddr *) &(link->clientAddress), &(link->clientLen));
    if (numBytesReceived < 0) {
        LogFatal("recvfrom() failed");
    }
    return numBytesReceived;
}

static int
_udp_send(Link *link, uint8_t *buffer, int length)
{
    int val = sendto(link->socket, buffer, length, 0,
        (struct sockaddr *) &link->clientAddress, link->clientLen);
    return val;
}

static int
_tcp_receive(Link *link, uint8_t *buffer)
{
    int recvMsgSize = recv(link->clientSocket, buffer, MTU, 0);
    return recvMsgSize;
}

static int
_tcp_send(Link *link, uint8_t *buffer, int length)
{
    int numSent = send(link->clientSocket, buffer, length, 0);
    return numSent;
}

static Link *
_create_link()
{
    Link *link = (Link *) malloc(sizeof(Link));

    link->port = 0;
    link->socket = 0;
    link->hostAddress = NULL;

    return link;
}

static Link *
_create_udp_link(char *address, int port)
{
    Link *link = _create_link();
    link->type = LinkType_UDP;
    link->port = port;
    link->receiveFunction = _udp_receive;
    link->sendFunction = _udp_send;
    link->clientLen = sizeof(link->clientAddress);

    if ((link->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LogFatal("socket() failed");
    }

    // set address reuse
    int optval = 1;
    setsockopt(link->socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval , sizeof(int));

    // initialize the address
    bzero((char *) &link->sourceAddress, sizeof(link->sourceAddress));
    link->sourceAddress.sin_family = AF_INET;
    link->sourceAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    link->sourceAddress.sin_port = htons(link->port);

    if (bind(link->socket, (struct sockaddr *) &(link->sourceAddress), sizeof(link->sourceAddress)) < 0) {
        LogFatal("bind() failed");
    }

    return link;
}

static Link *
_create_tcp_link(char *address, int port)
{
    Link *link = _create_link();
    link->type = LinkType_TCP;
    link->port = port;
    link->clientLen = sizeof(link->clientAddress);
    link->receiveFunction = _tcp_receive;
    link->sendFunction = _tcp_send;

    if ((link->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        LogFatal("socket() failed");
    }

    memset(&(link->sourceAddress), 0, sizeof(link->sourceAddress));
    link->sourceAddress.sin_family = AF_INET;
    link->sourceAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    link->sourceAddress.sin_port = htons(link->port);

    int on = 1;
    if (setsockopt(link->socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0) {
        LogFatal("setsockopt() failed");
    }

    if (bind(link->socket, (struct sockaddr *) &(link->sourceAddress), sizeof(link->sourceAddress)) < 0) {
        LogFatal("bind() failed");
    }

    if (listen(link->socket, MAX_NUMBER_OF_TCP_CONNECTIONS) < 0) {
        LogFatal("listen() failed");
    }

    if ((link->clientSocket = accept(link->socket, (struct sockaddr *) &(link->clientAddress), &link->clientLen)) < 0) {
        LogFatal("accept() failed");
    }

    return link;
}

Link *
link_Create(LinkType type, char *address, int port)
{
    switch (type) {
        case LinkType_UDP:
            return _create_udp_link(address, port);
        case LinkType_TCP:
            return _create_tcp_link(address, port);
        default:
            fprintf(stderr, "Error: invalid LinkType specified: %d", type);
            return NULL;
    }
}

int
link_Receive(Link *link, uint8_t *buffer)
{
    return link->receiveFunction(link, buffer);
}

int
link_Send(Link *link, uint8_t *buffer, int length)
{
    return link->sendFunction(link, buffer, length);
}
