#include "BaseSocket.h"
#include "EventDispatch.h"
#include <unordered_map>
#include <WS2tcpip.h>
#include "ZLogger.h"

typedef std::unordered_map<net_handle_t, CBaseSocket *> SocketMap;
SocketMap g_socket_map;

void AddBaseSocket(CBaseSocket *pSocket) {
    g_socket_map.insert(std::make_pair((net_handle_t) pSocket->GetSocket(), pSocket));
}

void RemoveBaseSocket(CBaseSocket *pSocket) {
    g_socket_map.erase((net_handle_t) pSocket->GetSocket());
}

CBaseSocket *FindBaseSocket(net_handle_t fd) {
    CBaseSocket *pSocket = NULL;
    SocketMap::iterator iter = g_socket_map.find(fd);
    if (iter != g_socket_map.end()) {
        pSocket = iter->second;
        //pSocket->AddRef();
    }

    return pSocket;
}

//////////////////////////////

CBaseSocket::CBaseSocket() {
    //log("CBaseSocket::CBaseSocket\n");
    m_socket = INVALID_SOCKET;
    m_state = SOCKET_STATE_IDLE;
    m_read_active = false;
    m_write_active = false;
}

CBaseSocket::~CBaseSocket() {
    //log("CBaseSocket::~CBaseSocket, socket=%d\n", m_socket);
}

int CBaseSocket::Listen(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    m_local_ip = server_ip;
    m_local_port = port;
    m_callback = callback;
    m_callback_data = callback_data;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        LogError("socket failed, err_code={}\n", _GetErrorCode());
        return NETLIB_ERROR;
    }

    setSoLinger(false, 0);
    setReuseAddr(true);
    setNonblock(true);
    setIntOption(SO_KEEPALIVE, 1);
    setIntOption(SO_SNDBUF, 640000);
    setIntOption(SO_RCVBUF, 640000);
    setTcpNoDelay(true);

    sockaddr_in serv_addr;
    _SetAddr(server_ip, port, &serv_addr);
    int ret = ::bind(m_socket, (sockaddr *) &serv_addr, sizeof(serv_addr));
    if (ret == SOCKET_ERROR) {
        LogError("bind failed, err_code={} ,  err={}", _GetErrorCode(), strerror(errno));
        closesocket(m_socket);
        return NETLIB_ERROR;
    }

    ret = listen(m_socket, 128 /*256*/);
    if (ret == SOCKET_ERROR) {
        LogError("listen failed, err_code={}", _GetErrorCode());
        closesocket(m_socket);
        return NETLIB_ERROR;
    }

    m_state = SOCKET_STATE_LISTENING;

    LogDebug("CBaseSocket::Listen on {}:{}", server_ip, port);

    AddBaseSocket(this);
    CEventDispatch::Instance()->AddEvent(*this, SOCKET_READ);
    return NETLIB_OK;
}

net_handle_t CBaseSocket::Connect(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    LogDebug("CBaseSocket::Connect, server_ip={}, port={}", server_ip, port);

    m_remote_ip = server_ip;
    m_remote_port = port;
    m_callback = callback;
    m_callback_data = callback_data;

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        LogError("socket failed, err_code={}", _GetErrorCode());
        return NETLIB_INVALID_HANDLE;
    }

    setNonblock(true);
    setTcpNoDelay(true);
    sockaddr_in serv_addr;
    _SetAddr(server_ip, port, &serv_addr);
    int ret = connect(m_socket, (sockaddr *) &serv_addr, sizeof(serv_addr));
    if ((ret == SOCKET_ERROR) && (!_IsBlock(_GetErrorCode()))) {
        LogError("connect failed, err_code={}", _GetErrorCode());
        closesocket(m_socket);
        return NETLIB_INVALID_HANDLE;
    }
    m_state = SOCKET_STATE_CONNECTING;
    AddBaseSocket(this);
    CEventDispatch::Instance()->AddEvent(*this, SOCKET_READ);
    CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);

    return (net_handle_t) m_socket;
}

int CBaseSocket::Send(void *buf, int len) {
    if (m_state != SOCKET_STATE_CONNECTED)
        return NETLIB_ERROR;

    int ret = send(m_socket, (char *) buf, len, 0);
    if (ret == SOCKET_ERROR) {
        int err_code = _GetErrorCode();
        if (_IsBlock(err_code)) {
            CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);
            ret = 0;
            //log("socket send block fd=%d", m_socket);
        } else {
            LogWarn("!!!send failed, error code: {}", err_code);
        }
    }

    return ret;
}

int CBaseSocket::Recv(void *buf, int len) {
    return recv(m_socket, (char *) buf, len, 0);
}

int CBaseSocket::Close() {
    CEventDispatch::Instance()->RemoveEvent(*this, SOCKET_CLOSE);
    RemoveBaseSocket(this);
    closesocket(m_socket);
    

    return 0;
}

void CBaseSocket::OnRead() {
    if (m_state == SOCKET_STATE_LISTENING) {
        _AcceptNewSocket();
    } else {
        u_long avail = 0;
        if ((ioctlsocket(m_socket, FIONREAD, &avail) == SOCKET_ERROR) || (avail == 0)) {
            LogDebug("base socket ioctlsocket failed, err_code={}, {}", _GetErrorCode(), strerror(errno));
            m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t) m_socket, NULL);
        } else {
            m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t) m_socket, NULL);
        }
    }
}

void CBaseSocket::OnWrite() {
#if ((defined _WIN32) || (defined __APPLE__))
    CBaseSocket &socket = *this;
    CEventDispatch::Instance()->RemoveEvent(socket, SOCKET_WRITE);
#endif

    if (m_state == SOCKET_STATE_CONNECTING) {
        int error = 0;
        socklen_t len = sizeof(error);
#ifdef _WIN32

        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
#else
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (void *) &error, &len);
#endif
        if (error) {
            LogWarn("getsockopt failed, err_code={}, {}", _GetErrorCode(), strerror(errno));
            m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t) m_socket, NULL);
        } else {
            m_state = SOCKET_STATE_CONNECTED;
            m_callback(m_callback_data, NETLIB_MSG_CONFIRM, (net_handle_t) m_socket, NULL);
        }
    } else {
        m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t) m_socket, NULL);
    }
}

void CBaseSocket::OnClose() {
    LogInfo("OnClose  socket ={}", m_socket);
    m_state = SOCKET_STATE_CLOSING;
    m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t) m_socket, NULL);
}

void CBaseSocket::SetSendBufSize(int send_size) {
    bool rc = setIntOption(SO_SNDBUF, send_size);
    if (rc == false) {
        LogError("set SO_SNDBUF failed for fd={}", m_socket);
    }
}

void CBaseSocket::SetRecvBufSize(int recv_size) {
    bool rc = setIntOption(SO_RCVBUF, recv_size);
    if (rc == false) {
        LogError("set SO_RCVBUF failed for fd={}", m_socket);
    }
}

bool CBaseSocket::setIntOption(int option, int value) {
    bool rc = false;
    if (_checkSocketHandle()) {
        rc = (setsockopt(m_socket, SOL_SOCKET, option,
                         (const char *) &value, sizeof(value)) == 0);
    }
    return rc;
}

bool CBaseSocket::_setTimeOption(int option, int sec, int usec) {
    bool rc = false;
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

    if (_checkSocketHandle()) {
        rc = (setsockopt(m_socket, SOL_SOCKET, option, (const char *) &tv, sizeof(tv)) == 0);
    }
    return rc;
}

bool CBaseSocket::_checkSocketHandle() {
    if (m_socket == -1) {
        return false;
    } else {
        return true;
    }
}

bool CBaseSocket::setSoLinger(bool doLinger, int seconds) {
    bool rc = false;
    struct linger lingerTime;

    lingerTime.l_onoff = doLinger ? 1 : 0;
    lingerTime.l_linger = seconds;
    if (_checkSocketHandle()) {
        rc = (setsockopt(m_socket, SOL_SOCKET, SO_LINGER,
                         (const char *) &lingerTime, sizeof(lingerTime)) == 0);
    }
    return rc;
}

int CBaseSocket::_GetErrorCode() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

bool CBaseSocket::_IsBlock(int error_code) {
#ifdef _WIN32
    return ( (error_code == WSAEINPROGRESS) || (error_code == WSAEWOULDBLOCK) );
#else
    return ((error_code == EINPROGRESS) || (error_code == EWOULDBLOCK));
#endif
}

void CBaseSocket::setNonblock(bool noBlock) {
#ifdef _WIN32
    u_long nonblock = noBlock ? 1 : 0;
    int ret = ioctlsocket(m_socket, FIONBIO, &nonblock);
#else
    int flags = fcntl(m_socket, F_GETFL);
    if (noBlock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    int ret = fcntl(m_socket, F_SETFL, flags);
#endif
    if (ret == SOCKET_ERROR) {
        LogError("setNonblock failed, err_code={}", _GetErrorCode());
    }
}

void CBaseSocket::setReuseAddr(bool on) {
    int reuse = on ? 1 : 0;

    if (false == setIntOption(SO_REUSEADDR, reuse)) {
        LogError("_setReuseAddr failed, err_code={}", _GetErrorCode());
    }
}

void CBaseSocket::setTcpNoDelay(bool noDelay) {
    int noDelayInt = noDelay ? 1 : 0;
    int ret = SOCKET_ERROR;

    if (_checkSocketHandle()) {
        ret = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,
                         (const char *) &noDelayInt, sizeof(noDelayInt));
    }
    if (ret == SOCKET_ERROR) {
        LogError("_setTcpNoDelay failed, err_code={}", _GetErrorCode());
    }
}

void CBaseSocket::_SetAddr(const char *ip, const uint16_t port, sockaddr_in *pAddr) {
    memset(pAddr, 0, sizeof(sockaddr_in));
    pAddr->sin_family = AF_INET;
    pAddr->sin_port = htons(port);
    pAddr->sin_addr.s_addr = inet_addr(ip);
    if (pAddr->sin_addr.s_addr == INADDR_NONE) {
        hostent *host = gethostbyname(ip);
        if (host == NULL) {
            LogError("gethostbyname failed, ip={}", ip);
            return;
        }

        pAddr->sin_addr.s_addr = *(uint32_t *) host->h_addr;
    }
}

int CBaseSocket::_AcceptNewSocket() {
    SOCKET fd = -1;
    sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(sockaddr_in);
    char ip_str[64];
    while ((fd = accept(m_socket, (sockaddr *) &peer_addr, &addr_len)) != INVALID_SOCKET) {
        CBaseSocket *pSocket = new CBaseSocket();
        uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
        uint16_t port = ntohs(peer_addr.sin_port);

        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);

        //log("AcceptNewSocket, socket=%d from %s:%d", fd, ip_str, port);

        pSocket->SetSocket(fd);
        pSocket->SetCallback(m_callback);
        pSocket->SetCallbackData(m_callback_data);
        pSocket->SetState(SOCKET_STATE_CONNECTED);
        pSocket->SetRemoteIP(ip_str);
        pSocket->SetRemotePort(port);

        pSocket->setTcpNoDelay(true);
        pSocket->setNonblock(true);
        AddBaseSocket(pSocket);
        CEventDispatch::Instance()->AddEvent(*pSocket, SOCKET_READ);
        m_callback(m_callback_data, NETLIB_MSG_CONNECT, (net_handle_t) fd, NULL);
    }
    return fd;
}