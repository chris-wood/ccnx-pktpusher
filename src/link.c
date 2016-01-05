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
    return 0;
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
    link->sourceAddress.sin_port = htons(port);

    // Set the link parameters
    link->port = port;
    link->clientLen = sizeof(link->clientAddress);

    if (bind(link->socket, (struct sockaddr *) &(link->sourceAddress), sizeof(link->sourceAddress)) < 0) {
        LogFatal("bind() failed");
    }

    link->receiveFunction = _udp_receive;
    link->sendFunction = _udp_send;

    return link;
}

static Link *
_create_tcp_link(char *address, int port)
{
    Link *link = _create_link();
    link->type = LinkType_TCP;

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
