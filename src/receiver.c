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

typedef struct {

    // TODO: generalize this into a link (UDP, TCP, etc...)
    int port;
    int socket;
    struct sockaddr_in serverAddress;
    char *hostAddress;

    PacketRepo *repo;
} UDPServer;

int
main(int argc, char **argv)
{
    socklen_t clientlen;
    struct sockaddr_in clientAddress;
    struct hostent *hostp;
    int numBytesReceived;
    UDPServer server;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server Port> <Directory>\n", argv[0]);
        exit(1);
    }

    // Setup the server port
    server.port = atoi(argv[1]);

    // Load the contents of the directory into memory
    server.repo = packetRepo_LoadFromFile(argv[2]);
    size_t numPackets = packetRepo_GetNumberOfPackets(server.repo);

    if ((server.socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LogFatal("socket() failed");
    }

    // set address reuse
    int optval = 1;
    setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval , sizeof(int));

    bzero((char *) &server.serverAddress, sizeof(server.serverAddress));
    server.serverAddress.sin_family = AF_INET;
    server.serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    server.serverAddress.sin_port = htons(server.port);

    if (bind(server.socket, (struct sockaddr *) &(server.serverAddress), sizeof(server.serverAddress)) < 0) {
        LogFatal("bind() failed");
    }

    clientlen = sizeof(clientAddress);
    uint8_t buffer[MTU];
    size_t numReceived = 0;
    while (numReceived < numPackets) {
        bzero(buffer, MTU);
        numBytesReceived = recvfrom(server.socket, buffer, MTU, 0, (struct sockaddr *) &clientAddress, &clientlen);
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
        Buffer *content = packetRepo_Lookup(server.repo, name, hash);
        if (content != NULL) {
#if DEBUG
            printf("Sending [%zu]:\n", content->length);
            // for (int i = 0; i < content->length; i++) {
            //     printf("%02x", content->bytes[i]);
            // }
            // printf("\n");
#endif
            if (sendto(server.socket, content->bytes, content->length, 0,
            	(struct sockaddr *) &clientAddress, clientlen) < 0) {
                LogFatal("Error sending content object response\n");
            }
        } else {
            LogFatal("Not found.\n");
        }

        numReceived++;
    }

    close(server.socket);

    return 0;
}
