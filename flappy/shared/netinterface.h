
#ifndef _netinterface_h
#define _netinterface_h

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define NETINTERFACE_USING_WINDOWS
#else
#define NETINTERFACE_USING_BSD
#endif

#ifdef NETINTERFACE_USING_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
typedef SOCKET NISocket;
typedef int NITransferSize;
typedef int NISockAddrSize;
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int NISocket;
typedef size_t NITransferSize;
typedef socklen_t NISockAddrSize;
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct SNetInterface {
    NISocket m_niPeer;
} NetInterface;
typedef struct SNetPeerAddress{
    struct sockaddr_in sin;

}NetPeerAddress;



typedef void (*NiTransferCallback)(const NetInterface* ni,  struct sockaddr* niAddr,
                                   const char* buffer, NITransferSize niTransferSize);

NetInterface* MakeNetInterface();
void DestroyNetInterface(NetInterface* ni);
int NIMakeSocket(NetInterface* ni);
int NIMakeSocketBind(NetInterface* ni);
void NetSleep(uint32_t u32Miliseconds);
void NIPoll(NetInterface* ni, NiTransferCallback niCallback);
void SendPacket(NetInterface* ni, struct sockaddr* niAddr, char* msgbuffer,
                uint32_t size);

NetPeerAddress NIMakeRemoteAddress(const char* cHostName, const char* port);
int IsErrorNoBlock(NetInterface* ni);
void NIDestroySocket(NetInterface* ni);

#endif
