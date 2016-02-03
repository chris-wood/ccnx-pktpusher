#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include "util.h"
#include "parser.h"
#include "repo.h"
#include "link.h"

int
main(int argc, char **argv)
{
    socklen_t clientlen;
    struct sockaddr_in clientAddress;
    struct hostent *hostp;
    int numBytesReceived;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server Port> <Data File>\n", argv[0]);
        exit(1);
    }

    // Setup the server port
    int port = atoi(argv[1]);

    // Load the contents of the directory into memory
    PacketRepo *repo = packetRepo_LoadFromFile(argv[2]);
    size_t numPackets = packetRepo_GetNumberOfPackets(repo);

    // Create the link
    // TODO: the address shouldn't be fixed
    Link *link = link_Listen(LinkType_TCP, "127.0.0.1", port);

    uint8_t buffer[MTU];
    size_t numReceived = 0;
    while (numReceived < numPackets) {
        bzero(buffer, MTU);
        numBytesReceived = link_Receive(link, buffer);
        if (numBytesReceived < 0) {
            LogFatal("recvfrom() failed");
        }

#if DEBUG
        printf("Received %d bytes\n", numBytesReceived);
#endif

        // 1. get the name of the packet
        uint16_t len = ((uint16_t)(buffer[2]) << 8) | (uint16_t)(buffer[3]);
        Buffer *name = _readName(buffer + 8, len - 8);
        Buffer *hash = _readContentObjectHash(buffer + 8, len - 8);

        // 2) index into the repo to get the packet
        Buffer *content = packetRepo_Lookup(repo, name, hash);
        if (content != NULL) {
#if DEBUG
            printf("Sending [%zu]:\n", content->length);
            for (int i = 0; i < content->length; i++) {
                printf("%02x", content->bytes[i]);
            }
            printf("\n");
#endif
            if (link_Send(link, content->bytes, content->length) < 0) {
                LogFatal("Error sending content object response\n");
            }
        } else {
            LogFatal("Not found");
        }

        numReceived++;
    }

    return 0;
}
