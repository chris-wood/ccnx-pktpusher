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
    LinkType_UDP,
    LinkType_TCP,
    LinkType_ETH,
    LinkType_Invalid
} LinkType;

Link *link_Create(LinkType type, char *address, int port);

int link_Receive(Link *link, uint8_t *buffer);
int link_Send(Link *link, uint8_t *buffer, int length);


#endif // link_h_
