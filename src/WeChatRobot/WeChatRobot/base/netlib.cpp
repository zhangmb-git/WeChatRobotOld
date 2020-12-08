#include "netlib.h"
#include "BaseSocket.h"
#include "SslSocket.h"

int netlib_init() {
    int ret = NETLIB_OK;
#ifdef _WIN32
    WSADATA wsaData;
    WORD wReqest = MAKEWORD(1, 1);
    if (WSAStartup(wReqest, &wsaData) != 0)
    {
        ret = NETLIB_ERROR;
    }
#endif
    CEventDispatch::Instance()->EventTimerInit();
    return ret;
}

int netlib_destroy() {
    int ret = NETLIB_OK;
#ifdef _WIN32
    if (WSACleanup() != 0)
    {
        ret = NETLIB_ERROR;
    }
#endif

    return ret;
}

int netlib_listen(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    CBaseSocket *pSocket = new CBaseSocket();
    if (!pSocket)
        return NETLIB_ERROR;

    int ret = pSocket->Listen(server_ip, port, callback, callback_data);
    if (ret == NETLIB_ERROR)
        delete pSocket;
    return ret;
}

int netlib_ssl_listen(
        const char *server_ip,
        uint16_t port,
        const char *cert_file,
        const char *key_file,
        const char *dh_file,
        callback_t callback,
        void *callback_data) {
    CSslSocket *pSocket = new CSslSocket(cert_file, key_file, dh_file);
    if (!pSocket)
        return NETLIB_ERROR;

    int ret = pSocket->Listen(server_ip, port, callback, callback_data);
    if (ret == NETLIB_ERROR)
        delete pSocket;
    return ret;
}

net_handle_t netlib_connect(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    CBaseSocket *pSocket = new CBaseSocket();
    if (!pSocket)
        return NETLIB_INVALID_HANDLE;

    net_handle_t handle = pSocket->Connect(server_ip, port, callback, callback_data);
    if (handle == NETLIB_INVALID_HANDLE)
        delete pSocket;
    return handle;
}

net_handle_t netlib_ssl_connect(const char *server_ip, uint16_t port, const char *cert_file,
                                callback_t callback,
                                void *callback_data) {
    CSslSocket *pSocket = new CSslSocket(cert_file, NULL, NULL);
    if (!pSocket)
        return NETLIB_ERROR;

    net_handle_t handle = pSocket->Connect(server_ip, port, callback, callback_data);
    if (handle == NETLIB_INVALID_HANDLE)
        delete pSocket;
    return handle;
}

int netlib_send(net_handle_t handle, void *buf, int len) {
    CBaseSocket *pSocket = FindBaseSocket(handle);
    if (!pSocket) {
        return NETLIB_ERROR;
    }
    int ret = pSocket->Send(buf, len);
    //pSocket->ReleaseRef();
    return ret;
}

int netlib_recv(net_handle_t handle, void *buf, int len) {
    CBaseSocket *pSocket = FindBaseSocket(handle);
    if (!pSocket)
        return NETLIB_ERROR;

    int ret = pSocket->Recv(buf, len);
    //pSocket->ReleaseRef();
    return ret;
}

int netlib_close(net_handle_t handle) {
    CBaseSocket *pSocket = FindBaseSocket(handle);
    if (!pSocket)
        return NETLIB_ERROR;

    int ret = pSocket->Close();
    //pSocket->ReleaseRef();
    return ret;
}

int netlib_option(net_handle_t handle, int opt, void *optval) {
    CBaseSocket *pSocket = FindBaseSocket(handle);
    if (!pSocket)
        return NETLIB_ERROR;

    if ((opt >= NETLIB_OPT_GET_REMOTE_IP) && !optval)
        return NETLIB_ERROR;

    switch (opt) {
        case NETLIB_OPT_SET_CALLBACK:
            pSocket->SetCallback((callback_t) optval);
            break;
        case NETLIB_OPT_SET_CALLBACK_DATA:
            pSocket->SetCallbackData(optval);
            break;
        case NETLIB_OPT_GET_REMOTE_IP:
            *(std::string *) optval = pSocket->GetRemoteIP();
            break;
        case NETLIB_OPT_GET_REMOTE_PORT:
            *(uint16_t *) optval = pSocket->GetRemotePort();
            break;
        case NETLIB_OPT_GET_LOCAL_IP:
            *(std::string *) optval = pSocket->GetLocalIP();
            break;
        case NETLIB_OPT_GET_LOCAL_PORT:
            *(uint16_t *) optval = pSocket->GetLocalPort();
            break;
        case NETLIB_OPT_SET_SEND_BUF_SIZE:
            pSocket->SetSendBufSize(*(uint32_t *) optval);
            break;
        case NETLIB_OPT_SET_RECV_BUF_SIZE:
            pSocket->SetRecvBufSize(*(uint32_t *) optval);
            break;
    }

    //pSocket->ReleaseRef();
    return NETLIB_OK;
}

#if 0
int netlib_register_timer(callback_t callback, void* user_data, uint64_t interval)
{
    CEventDispatch::Instance()->AddTimer(callback, user_data, interval);
    return 0;
}

int netlib_delete_timer(callback_t callback, void* user_data)
{
    CEventDispatch::Instance()->RemoveTimer(callback, user_data);
    return 0;
}
#endif //0

int netlib_register_timer(rb_timer_item *pTimerItem) {
    CEventDispatch::Instance()->AddTimer(pTimerItem);
    return 0;
}

int netlib_delete_timer(rb_timer_item *pTimerItem) {
    CEventDispatch::Instance()->RemoveTimer(pTimerItem);
    return 0;
}

int netlib_cancle_timer() {
    CEventDispatch::Instance()->CancelTimers();
    return 0;
}

int netlib_add_loop(callback_t callback, void *user_data) {
    CEventDispatch::Instance()->AddLoop(callback, user_data);
    return 0;
}

void netlib_eventloop(uint32_t wait_timeout) {
    CEventDispatch::Instance()->StartDispatch(wait_timeout);
}

void netlib_stop_event() {
    CEventDispatch::Instance()->StopDispatch();
}

bool netlib_is_running() {
    return CEventDispatch::Instance()->isRunning();
}
