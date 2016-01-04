#ifndef repo_h_
#define repo_h_

#include "util.h"

struct packet_repo;
typedef struct packet_repo PacketRepo;

PacketRepo *packetRepo_LoadFromFile(char *file);
Buffer *packetRepo_Lookup(PacketRepo *repo, Buffer *name, Buffer *hash);
size_t packetRepo_GetNumberOfPackets(PacketRepo *repo);

#endif // repo_h_
