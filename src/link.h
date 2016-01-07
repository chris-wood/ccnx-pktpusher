#ifndef link_h_
#define link_h_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct link;
typedef struct link Link;

typedef enum {
    LinkType_UDP = 0,
    LinkType_TCP = 1,
    LinkType_ETH = 2,
    LinkType_Invalid = 3
} LinkType;

Link *link_Listen(LinkType type, char *address, int port);
Link *link_Connect(LinkType type, char *address, int port);

void link_SetTimeout(Link *link, struct timeval tv);

int link_Receive(Link *link, uint8_t *buffer);
int link_Send(Link *link, uint8_t *buffer, int length);


#endif // link_h_
