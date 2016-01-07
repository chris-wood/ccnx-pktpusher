#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "repo.h"
#include "parser.h"

struct keyvalue;
typedef struct keyvalue {
    uint8_t *key;
    size_t keylen;
    Buffer *value;
    struct keyvalue *next;
} KeyValue;

struct packet_repo {
    size_t numberOfPackets;
    KeyValue *head;
};

static void
_testParsePacket()
{
    FILE *fp = fopen("data_int", "rb");

    uint8_t header[8];

    if (fp != NULL) {
        // read the header
        fread(header, 1, 8, fp);

        // printf("%d %d\n", header[2], header[3]);

        // get the size
        uint16_t len = ((uint16_t)(header[2]) << 8) | (uint16_t)(header[3]);

        // read the packet into a Buffer
        Buffer *pktBuffer = malloc(sizeof(Buffer));
        pktBuffer->bytes = malloc(len);
        pktBuffer->length = len;
        int numRead = fread(pktBuffer->bytes, 1, len, fp);

        Buffer *name = _readName(pktBuffer->bytes, pktBuffer->length);
        for (size_t i = 0; i < name->length; i++) {
            putc(name->bytes[i], stdout);
        }
    }
}

static Buffer *
_loadContent(PacketRepo *repo, Buffer *name, Buffer *hash)
{
    KeyValue *curr = repo->head;
    while (curr != NULL) {
        if (curr->keylen == name->length) {
            if (memcmp(curr->key, name->bytes, curr->keylen) == 0) {
                return curr->value;
            }
        }
        curr = curr->next;
    }
    return NULL;
}

static void
_displayBuffer(Buffer *Buffer)
{
    for (int i = 0; i < Buffer->length; i++) {
        putc(Buffer->bytes[i], stdout);
    }
}

static KeyValue *
_loadDataFromFile(char *fname)
{
    FILE *fp = fopen(fname, "rb");

    if (fp == NULL) {
        return 0;
    }

    uint8_t header[8];
    size_t numRead = 1;
    size_t numPackets = 0;

    KeyValue *head = NULL;
    while (numRead > 0) {
        numRead = fread(header, 1, 8, fp);
        if (numRead == 0) {
            break;
        }

        uint16_t len = ((uint16_t)(header[2]) << 8) | (uint16_t)(header[3]);
        Buffer *pktBuffer = malloc(sizeof(Buffer));
        pktBuffer->bytes = malloc(len); // allocate packet header room
        pktBuffer->length = len;
        memcpy(pktBuffer->bytes, header, 8); // move the header into the packet
        int numRead = fread(pktBuffer->bytes + 8, 1, len - 8, fp);

        // Extract the name and hash
        Buffer *name = _readName(pktBuffer->bytes + 8, len - 8);
        Buffer *hash = _readContentObjectHash(pktBuffer->bytes + 8, len - 8);

        KeyValue *kv = (KeyValue *) malloc(sizeof(KeyValue));
        kv->key = malloc(name->length);
        memcpy(kv->key, name->bytes, name->length);
        kv->keylen = name->length;
        kv->value = pktBuffer;

#if DEBUG
        printf("Repo addition:");
        _displayBuffer(name);
        printf("\n");
#endif
        // for (int i = 0; i < pktBuffer->length; i++) {
        //     printf("%02x", pktBuffer->bytes[i]);
        // }
        // printf("\n");

        kv->next = head;
        head = kv;

        numPackets++;
    }

    return head;
}

PacketRepo *
packetRepo_LoadFromFile(char *file)
{
    PacketRepo *repo = (PacketRepo *) malloc(sizeof(PacketRepo));
    repo->head = _loadDataFromFile(file);

    size_t count = 0;
    KeyValue *curr = repo->head;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    repo->numberOfPackets = count;

    return repo;
}

Buffer *
packetRepo_Lookup(PacketRepo *repo, Buffer *name, Buffer *hash)
{
    return _loadContent(repo, name, hash);
}

size_t
packetRepo_GetNumberOfPackets(PacketRepo *repo)
{
    return repo->numberOfPackets;
}
