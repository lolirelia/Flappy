
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
typedef struct sockaddr_in NISockAddrIn;
typedef struct sockaddr NISockAddr;

typedef struct SNetInterface {
    NISocket m_niPeer;
} NetInterface;
typedef struct SNetInterfaceAddr {
    NISockAddrIn m_niRemoteAddr;
    NISockAddrSize m_niUSizeOfAddr;
} NetInterfaceAddr;

typedef void (*NiTransferCallback)(NetInterface* ni, NetInterfaceAddr* niAddr,
                                   char* buffer, NITransferSize niTransferSize);

NetInterface* MakeNetInterface();
void DestroyNetInterface(NetInterface* ni);
int MakeSocket(NetInterface* ni);
int MakeSocketBind(NetInterface* ni);
void NetSleep(uint32_t u32Miliseconds);
void Poll(NetInterface* ni, NiTransferCallback niCallback);
void SendPacket(NetInterface* ni, NetInterfaceAddr* niAddr, char* buffer);

NetInterfaceAddr* MakeRemoteAddress(const char* cHostName, uint16_t u16Port);
void DestroyNetInterfaceAddr(NetInterfaceAddr* niAddr);

int IsErrorNoBlock(NetInterface* ni);
void DestroySocket(NetInterface* ni);

#endif
