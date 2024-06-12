
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
    NISockAddrIn niSelfAddress = {0};
    niSelfAddress.sin_family = AF_INET;
    niSelfAddress.sin_port = htons(23456);
    niSelfAddress.sin_addr.s_addr = INADDR_ANY;
    if (!NIMakeSocket(ni)) {
        return 0;
    }

    if (bind(ni->m_niPeer, (NISockAddr*)&niSelfAddress, sizeof(NISockAddrIn)) !=
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
    NetInterfaceAddr niAddr = {0};
    NISockAddrSize niUSizeOfAddr = sizeof(NISockAddrIn);

    NITransferSize niTransferSize =
        recvfrom(ni->m_niPeer, msgbuffer, sizeof(msgbuffer), 0,
                 (NISockAddr*)&niAddr.m_niRemoteAddr, &niUSizeOfAddr);

    if (!IsErrorNoBlock(ni)) {
        niAddr.m_niUSizeOfAddr = niUSizeOfAddr ;
        niCallback(ni, &niAddr, msgbuffer, niTransferSize);
    }
}
void SendPacket(NetInterface* ni, NetInterfaceAddr* niAddr, char* msgbuffer,
                uint32_t size) {
    assert(IsValidSocket(ni));
    NITransferSize niTransferSize =
        sendto(ni->m_niPeer, msgbuffer, size, 0,
               (NISockAddr*)&niAddr->m_niRemoteAddr, niAddr->m_niUSizeOfAddr);
    if (niTransferSize < 0) {
        if (!IsErrorNoBlock(ni)) {
        }
    }
}

NetInterfaceAddr* MakeRemoteAddress(const char* cHostName, uint16_t u16Port) {
    NetInterfaceAddr* niAddr = malloc(sizeof(NetInterfaceAddr));
    assert(niAddr != NULL);
#ifdef NETINTERFACE_USING_WINDOWS
    InetPtonA(AF_INET, cHostName, &niAddr.m_niRemoteAddr.sin_addr);
#else
    inet_aton(cHostName, &niAddr->m_niRemoteAddr.sin_addr);
#endif
    niAddr->m_niRemoteAddr.sin_port = htons(u16Port);
    niAddr->m_niRemoteAddr.sin_family = AF_INET;
    niAddr->m_niUSizeOfAddr = sizeof(NetInterfaceAddr);
    return niAddr;
}
void DestroyNetInterfaceAddr(NetInterfaceAddr* niAddr) {
    assert(niAddr != NULL);
    free(niAddr);
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
