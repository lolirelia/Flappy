
#include "netinterface.h"
static int IsValidSocket(NetInterface* ni) {
#ifdef NETINTERFACE_USING_WINDOWS
    if (ni->m_niPeer == INVALID_SOCKET) return 0;
#else
    if (ni->m_niPeer == -1) return 0;
#endif
    return 1;
}
void NetSleep(uint32_t u32Miliseconds) {
#ifdef NETINTERFACE_USING_WINDOWS
    Sleep(u32Miliseconds);
#else
    u32Miliseconds *= 1000;
    usleep(u32Miliseconds);
#endif
}
static int NonBlock(NetInterface* ni) {
#ifdef NETINTERFACE_USING_WINDOWS
    int timeout = 1;
    if (setsockopt(ni->m_niPeer, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                   sizeof(int)) != 0) {
        NIDestroySocket(ni);
        return 0;
    }
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    if (setsockopt(ni->m_niPeer, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) !=
        0) {
        NIDestroySocket(ni);
        return 0;
    }
#endif
    return 1;
}
NetInterface* MakeNetInterface() {
    NetInterface* ni = malloc(sizeof(NetInterface));
    assert(ni != NULL);

#ifdef NETINTERFACE_USING_WINDOWS
    WSAData data = {0};
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
    return ni;
}
void DestroyNetInterface(NetInterface* ni) {
    assert(IsValidSocket(ni) == 0);

#ifdef NETINTERFACE_USING_WINDOWS
    WSACleanup();
#endif
    free(ni);
}

int NIMakeSocket(NetInterface* ni) {
    ni->m_niPeer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (!IsValidSocket(ni)) return 0;

    int enable = 1;
    if (setsockopt(ni->m_niPeer, SOL_SOCKET, SO_REUSEADDR, (char*)&enable,
                   sizeof(int)) != 0) {
        NIDestroySocket(ni);
        return 0;
    }
    NonBlock(ni);
    return 1;
}

int NIMakeSocketBind(NetInterface* ni) {
    struct sockaddr_in niSelfAddress = {0};
    niSelfAddress.sin_family = AF_INET;
    niSelfAddress.sin_port = htons(23456);
    niSelfAddress.sin_addr.s_addr = INADDR_ANY;
    if (!NIMakeSocket(ni)) {
        return 0;
    }

    if (bind(ni->m_niPeer, (struct sockaddr*)&niSelfAddress, sizeof(struct sockaddr)) !=
        0) {
        NIDestroySocket(ni);
        return 0;
    }
    return 1;
}
void NIPoll(NetInterface* ni, NiTransferCallback niCallback) {
    assert(IsValidSocket(ni));

    char msgbuffer[128];
    memset(msgbuffer, 0, sizeof(msgbuffer));
    NetPeerAddress npa = {0};
    npa.size = sizeof(struct sockaddr);
    struct sockaddr_in* saddr;
    
    NITransferSize niTransferSize =
        recvfrom(ni->m_niPeer, msgbuffer, sizeof(msgbuffer), 0,
                 &npa.saddr, &npa.size);

    if (!IsErrorNoBlock(ni)) {
        niCallback(ni, &npa, msgbuffer, niTransferSize);
    }
}
#include <stdio.h>
void SendPacket(NetInterface* ni, struct sockaddr* niAddr, char* msgbuffer,
                uint32_t size) {
    assert(IsValidSocket(ni));
    struct addrinfo info;
    info.ai_protocol = IPPROTO_UDP;
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    struct addrinfo* result = NULL;
    assert(getaddrinfo("127.0.0.1","23456", &info, &result) == 0);
    NITransferSize niTransferSize =
        sendto(ni->m_niPeer, msgbuffer, size, 0,
               result->ai_addr,result->ai_addrlen);
    if (niTransferSize < 0) {
        if (!IsErrorNoBlock(ni)) {
        }
    }else if(niTransferSize > 0){
        printf("%zu\n",niTransferSize);
    }
    freeaddrinfo(result);
    
}
NetPeerAddress NIMakeRemoteAddress(const char* cHostName, const char* port) {
    NetPeerAddress npa;
    memset(&npa,0,sizeof(NetPeerAddress));
    struct addrinfo info;
    info.ai_protocol = IPPROTO_UDP;
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    struct addrinfo* result = NULL;
    assert(!getaddrinfo(cHostName, port, &info, &result));
    struct sockaddr_in sin;
    
    memcpy(&npa.sin,result->ai_addr,result->ai_addrlen);
    return npa;
}

uint32_t GetError(NetInterface* ni) {
#ifdef NETINTERFACE_USING_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}
int IsErrorNoBlock(NetInterface* ni) {
    assert(ni != NULL);
    assert(IsValidSocket(ni));
    int error = GetError(ni);
#ifdef NETINTERFACE_USING_WINDOWS
    if (error == WSAEWOULDBLOCK) {
        return 1;
    }
#else
    if (error == EWOULDBLOCK) {
        return 1;
    }
#endif
    return 0;
}
void NIDestroySocket(NetInterface* ni) {
    assert(IsValidSocket(ni));
#ifdef NETINTERFACE_USING_WINDOWS
    closesocket(ni->m_niPeer);
    ni->m_niPeer = INVALID_SCKET;
#else
    close(ni->m_niPeer);
    ni->m_niPeer = -1;
#endif
}
