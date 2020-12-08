#include "WinSock2.h"
#include "SslSocket.h"
#include "file_util.h"
#include <WS2tcpip.h>
#include "ZLogger.h"

int CSslSocket::s_ssl_initialized;
int CSslSocket::s_ssl_connection_index;
#define MAX_SSL_HANDSHAKE_DURATION    60000 // in ms

CSslSocket::CSslSocket(const char *cert, const char *key, const char *dh) {
    if (cert != NULL) m_cert = cert;
    if (key != NULL) m_key = key;
    if (dh != NULL) m_dh = dh;
    m_ssl_ctx = NULL;
    m_ssl = NULL;
    m_bio.p = NULL;

    if (s_ssl_initialized == 0) {
#ifndef OPENSSL_IS_BORINGSSL
        ::OPENSSL_config(NULL);
#endif
        ::SSL_library_init();
        ::SSL_load_error_strings();
        ::OpenSSL_add_all_algorithms();

#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#ifndef SSL_OP_NO_COMPRESSION
        {
        /*
         * Disable gzip compression in OpenSSL prior to 1.0.0 version,
         * this saves about 522K per connection.
         */
        int                  n;
        STACK_OF(SSL_COMP)  *ssl_comp_methods;

        ssl_comp_methods = ::SSL_COMP_get_compression_methods();
        n = sk_SSL_COMP_num(ssl_comp_methods);

        while (n--) {
            (void) sk_SSL_COMP_pop(ssl_comp_methods);
        }
        }
#endif
#endif
        s_ssl_connection_index = ::SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);

        if (s_ssl_connection_index == -1) {
            logSslError("SSL_get_ex_new_index");
        }
        s_ssl_initialized = 1;
    }
    m_ssl_ctx = ::SSL_CTX_new(::SSLv23_method());
    if (m_ssl_ctx == NULL) {
        logSslError("SSL_CTX_new");
    } else {
        ::SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_AUTO_RETRY);
        ::SSL_CTX_set_mode(m_ssl_ctx,
                           SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER); // must be set, or will get "error:1409F07F:SSL routines:SSL3_WRITE_PENDING: bad write retry"
        //::SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
        m_ownCtx = true;
#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
        /* this option allow a potential SSL 2.0 rollback (CAN-2005-2969) */
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_MSIE_SSLV2_RSA_PADDING);
#endif

#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
#endif

#ifdef SSL_OP_TLS_D5_BUG
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

        //::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_CTRL_CLEAR_OPTIONS
        /* only in 0.9.8m+ */
        ::SSL_CTX_clear_options(m_ssl_ctx,
                                SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
#endif

#ifdef SSL_OP_NO_TLSv1_1
        ::SSL_CTX_clear_options(m_ssl_ctx, SSL_OP_NO_TLSv1_1);
#endif

#ifdef SSL_OP_NO_TLSv1_2
        ::SSL_CTX_clear_options(m_ssl_ctx, SSL_OP_NO_TLSv1_2);
#endif

#ifdef SSL_OP_NO_SSLv2
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_SSLv2);
#endif

#ifdef SSL_OP_NO_COMPRESSION
        ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
        ::SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
        ::SSL_CTX_set_mode(m_ssl_ctx, SSL_MODE_NO_AUTO_CHAIN);
#endif

        ::SSL_CTX_set_read_ahead(m_ssl_ctx, 1);
        ::SSL_CTX_set_info_callback(m_ssl_ctx, sslInfoCallback);


    }
}

CSslSocket::CSslSocket(SSL_CTX *ctx, SOCKET sock) {
    m_socket = sock;
    m_ssl_ctx = ctx;
    m_ownCtx = false;
    m_isHandshaked = false;
    m_renegotiation = false;
    m_io_state = SSL_IO_NORMAL;
    m_ssl = NULL;
    m_bio.p = NULL;
    rbtimer_init(&m_handshakeTimeout, CSslSocket::handshakeTimeout, (void *) this, MAX_SSL_HANDSHAKE_DURATION, 0, 1);
    //DEBUG("###Initialise handshakeTimeout,which timer %p :%d",&m_handshakeTimeout,m_handshakeTimeout.interval);
}

CSslSocket::~CSslSocket() {
    if (m_ssl) {
        ::SSL_shutdown(m_ssl);
        if (m_bio.p != NULL) {
            m_bio.p = NULL; // SSL_free will call BIO_free
        }
        ::SSL_free(m_ssl);
        m_ssl = NULL;
    }
    if (m_ssl_ctx && m_ownCtx == true) {
        ::SSL_CTX_free(m_ssl_ctx);
    }
}

#define MAX_ERRSTR    1024

void CSslSocket::logSslError(const char *op) {
    char errstr[MAX_ERRSTR];
    char *p, *last;
    unsigned long n;
    const char *data;
    int flags;

    last = errstr + MAX_ERRSTR;
    p = errstr;
    *p = '\0';

    for (;;) {
        n = ERR_peek_error_line_data(NULL, NULL, &data, &flags);
        if (n == 0) {
            break;
        }

        if (p >= last) {
            goto next;
        }
        *p++ = ' ';
        ERR_error_string_n(n, p, last - p);
        while (p < last && *p) {
            p++;
        }
        if (p < last && *data && (flags & ERR_TXT_STRING)) {
            *p++ = ':';
            while (p < last) {
                *p = *data;
                if (*p == '\0') {
                    break;
                }
                p++;
                data++;
            }
        }
        next:
        (void) ERR_get_error();
    }
    last[-1] = '\0';
    LogError("{} failed:{}", op, errstr);
}

void CSslSocket::sslInfoCallback(const SSL *ssl, int where, int ret) {
    CSslSocket *pSocket;
    (void) ret;

    if (where & SSL_CB_HANDSHAKE_START) {
        pSocket = (CSslSocket *) ::SSL_get_ex_data(ssl, s_ssl_connection_index);
        if (pSocket != NULL && pSocket->m_isHandshaked) {
            pSocket->m_renegotiation = true;
        }
    }
}

int CSslSocket::Listen(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    if (m_ssl_ctx == NULL ||
        m_cert.empty() || m_cert.length() == 0 || !PathUtil::exist(m_cert.c_str()) ||
        m_key.empty() || m_key.length() == 0 || !PathUtil::exist(m_key.c_str())) {
        return NETLIB_ERROR;
    }
    int rc = CBaseSocket::Listen(server_ip, port, callback, callback_data);
    if (rc == NETLIB_OK) {
        bool use_dh = false;

        if (!m_dh.empty() && m_dh.length() > 0 && PathUtil::exist(m_dh.c_str())) {
            ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);
            use_dh = true;
        }
        if (::SSL_CTX_use_certificate_file(m_ssl_ctx, m_cert.c_str(), SSL_FILETYPE_PEM) != 1) {
            LogError("SSL_CTX_use_certificate_file failed: {}", ::ERR_reason_error_string(::ERR_get_error()));
            return NETLIB_ERROR;
        }

        if (::SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_key.c_str(), SSL_FILETYPE_PEM) != 1) {
            LogError("SSL_CTX_use_PrivateKey_file failed: {}", ::ERR_reason_error_string(::ERR_get_error()));
            return NETLIB_ERROR;
        }
        if (use_dh) {
            ::SSL_CTX_set_options(m_ssl_ctx, SSL_OP_SINGLE_DH_USE);
            ScopedBio bio = {::BIO_new_file(m_dh.c_str(), "r")};
            if (bio.p == NULL) {
                LogError("BIO_new_file failed: {}",::ERR_reason_error_string(::ERR_get_error()));
                return NETLIB_ERROR;
            }
            ScopedDh dh = {::PEM_read_bio_DHparams(bio.p, 0, 0, 0)};
            if (dh.p == NULL) {
                LogError("PEM_read_bio_DHparams failed: {}", ::ERR_reason_error_string(::ERR_get_error()));
                return NETLIB_ERROR;
            }
            ::SSL_CTX_set_tmp_dh(m_ssl_ctx, dh.p);
        }
        return NETLIB_OK;
    }
    return NETLIB_ERROR;
}

net_handle_t CSslSocket::Connect(const char *server_ip, uint16_t port, callback_t callback, void *callback_data) {
    if (m_cert.empty() || m_cert.length() == 0 || !PathUtil::exist(m_cert.c_str())) {
        LogError("CSslSocket::Connect, invalid certificate file");
        return NETLIB_INVALID_HANDLE;
    }
    ::SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_PEER, SSL_CTX_get_verify_callback(m_ssl_ctx));
    if (::SSL_CTX_load_verify_locations(m_ssl_ctx, m_cert.c_str(), 0) != 1) {
        LogError("SSL_CTX_load_verify_locations failed: {}", ERR_reason_error_string(ERR_get_error()));
        return NETLIB_INVALID_HANDLE;
    }
    m_remote_ip = server_ip;
    m_remote_port = port;
    m_callback = callback;
    m_callback_data = callback_data;
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        LogError("CSslSocket::Connect socket failed: {}", ::strerror(errno));
        return NETLIB_INVALID_HANDLE;
    }
    sockaddr_in serv_addr;
    _SetAddr(server_ip, port, &serv_addr);
    int ret = connect(m_socket, (sockaddr *) &serv_addr, sizeof(sockaddr_in));
    if ((ret == SOCKET_ERROR) && (!_IsBlock(_GetErrorCode()))) {
        LogError("CSslSocket::Connect connect failed: {}", ::strerror(errno));
        ::closesocket(m_socket);
        return NETLIB_INVALID_HANDLE;
    }
    m_bio.p = ::BIO_new_socket(m_socket, BIO_NOCLOSE);
    m_ssl = ::SSL_new(m_ssl_ctx);
    if (m_ssl == NULL) {
        LogError("SSL_new failed: {}", ::ERR_reason_error_string(::ERR_get_error()));
        ::closesocket(m_socket);
        return NETLIB_INVALID_HANDLE;
    }
    ::SSL_set_bio(m_ssl, m_bio.p, m_bio.p);
    if (::SSL_connect(m_ssl) != 1) {
        LogError("SSL_connect failed: {}", ::ERR_reason_error_string(::ERR_get_error()));
        ::closesocket(m_socket);
        return NETLIB_INVALID_HANDLE;
    }
    setNonblock(true);
    setTcpNoDelay(true);
    m_state = SOCKET_STATE_CONNECTING;
    AddBaseSocket(this);
    CEventDispatch::Instance()->AddEvent(*this, SOCKET_READ);
    CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);
    return (net_handle_t) m_socket;
}

int CSslSocket::Send(void *buf, int len) {
    if (m_state != SOCKET_STATE_CONNECTED) {
        //DEBUG("m_state = %d", m_state);
        return NETLIB_ERROR;
    }

    if (m_ssl == NULL) {
        //DEBUG("m_ssl == NULL");
        return NETLIB_ERROR;
    }

    while (::ERR_peek_error()) {
        logSslError("Send ignore stale global SSL error");
    }
    ::ERR_clear_error();

    int n = ::SSL_write(m_ssl, buf, len);
    if (n > 0) {
        if (m_io_state == SSL_IO_WANT_READ) {
            m_io_state = SSL_IO_NORMAL;
            //DEBUG("Send: WANT_READ fulfilled");
        } else if (m_io_state == SSL_IO_WANT_READ_STACKED) {
            m_io_state = SSL_IO_NORMAL;
			//DEBUG("Send: WANT_READ fulfilled, call stacked read handler");
            m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t) m_socket, NULL);
        }
        return n;
    }

    int sslerr = ::SSL_get_error(m_ssl, n);
    if (sslerr == SSL_ERROR_WANT_WRITE) {
        //DEBUG("sslerr == SSL_ERROR_WANT_WRITE");
        CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);
        return -EAGAIN;
    }

    if (sslerr == SSL_ERROR_WANT_READ) {
		//DEBUG("Send: peer started SSL renegotiation");
        if (m_read_active) {
            m_io_state = SSL_IO_WANT_READ_STACKED;
        } else {
            m_io_state = SSL_IO_WANT_READ;
            CEventDispatch::Instance()->AddEvent(*this, SOCKET_READ);
        }
        return -EAGAIN;
    }

    logSslError("SSL_write");
    return NETLIB_ERROR;
}

int CSslSocket::Recv(void *buf, int len) {
    if (m_ssl == NULL)
        return NETLIB_ERROR;

    if (m_renegotiation) {
        //DEBUG("SSL renegotiation disabled!!!");
        return NETLIB_ERROR;
    }

    while (::ERR_peek_error()) {
        logSslError("Recv ignore stale global SSL error");
    }
    ::ERR_clear_error();

    int n = ::SSL_read(m_ssl, buf, len);

    if (n > 0) {
        if (m_io_state == SSL_IO_WANT_WRITE) {
            m_io_state = SSL_IO_NORMAL;
            //DEBUG("Recv: WANT_WRITE fulfilled");
        } else if (m_io_state == SSL_IO_WANT_WRITE_STACKED) {
            m_io_state = SSL_IO_NORMAL;
			//DEBUG("Recv: WANT_WRITE fulfilled, call stacked write handler");
            m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t) m_socket, NULL);
        }
        return n;
    }

    int sslerr = ::SSL_get_error(m_ssl, n);
    if (sslerr == SSL_ERROR_WANT_READ) {
        return -EAGAIN;
    }

    if (sslerr == SSL_ERROR_WANT_WRITE) {
        if (m_write_active) {
            m_io_state = SSL_IO_WANT_WRITE_STACKED;
        } else {
            m_io_state = SSL_IO_WANT_WRITE;
            CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);
        }
        return -EAGAIN;
    }

    logSslError("SSL_read");
    return -EIO;
}

int CSslSocket::Close() {
    if (m_ssl) ::SSL_shutdown(m_ssl);
    CBaseSocket::Close();
    //DEBUG("###delete timer %p  %d for closing socket",&m_handshakeTimeout,m_handshakeTimeout.interval);
    netlib_delete_timer(&m_handshakeTimeout);
    return 0;
}

void CSslSocket::handshakeTimeout(void *data, uint8_t msg, uint32_t handle, void *pParam) {
    (void) msg;
    (void) handle;
    (void) pParam;

    if (data != NULL) {
        CSslSocket *pSocket = (CSslSocket *) data;
        if (pSocket->m_state == SOCKET_STATE_WAIT_HANDSHAKE
            || pSocket->m_state == SOCKET_STATE_HANDSHAKING) {
			//DEBUG("SSL Handshake timeout, fd:%d, peer addr(%s:%d)",pSocket->m_socket, pSocket->m_remote_ip.c_str(), (int) pSocket->m_remote_port);
            //CEventDispatch::Instance()->RemoveTimer(CSslSocket::handshakeTimeout, data);
            pSocket->Close();
        }
    }
}

void CSslSocket::cleanup() {
    //CEventDispatch::Instance()->RemoveTimer(CSslSocket::handshakeTimeout, this);
    Close();
}

int CSslSocket::doHandshake() {
    while (::ERR_peek_error()) {
        logSslError("Ignoring stale global error");
    }
    ::ERR_clear_error();

    int n = ::SSL_do_handshake(m_ssl);

    if (n == 1) {
        m_isHandshaked = true;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#ifdef SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS
        /* initial handshake done, disable renegotiation (CVE-2009-3555) */
        if (m_ssl->s3) {
            m_ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
        }

#endif
#endif
        return 0;
    }
    int sslerr = ::SSL_get_error(m_ssl, n);
    if (sslerr == SSL_ERROR_WANT_READ) {
        return EAGAIN;
    } else if (sslerr == SSL_ERROR_WANT_WRITE) {
        CEventDispatch::Instance()->AddEvent(*this, SOCKET_WRITE);
        return EAGAIN;
    } else if (sslerr == SSL_ERROR_SYSCALL) {
        //DEBUG("SSL_ERROR_SYSCALL:%s", strerror(errno));
    } else {
        logSslError("SSL_do_handshake");
    }
    return EIO;
}

void CSslSocket::onSslHandshaked() {
    SetState(SOCKET_STATE_CONNECTED);
    m_io_state = SSL_IO_NORMAL;
    m_callback(m_callback_data, NETLIB_MSG_CONNECT, (net_handle_t) m_socket, NULL);
    //CEventDispatch::Instance()->RemoveTimer(CSslSocket::handshakeTimeout, this);
    //DEBUG("###delete timer %p %d for ssl handshake successfully",&m_handshakeTimeout,m_handshakeTimeout.interval);
    netlib_delete_timer(&m_handshakeTimeout);
    if (m_write_active) {
        CEventDispatch::Instance()->RemoveEvent(*this, SOCKET_WRITE);
    }
}

void CSslSocket::OnRead() {
    switch (m_state) {
        case SOCKET_STATE_LISTENING: {
            SOCKET fd = -1;
            sockaddr_in peer_addr;
            socklen_t addr_len = sizeof(sockaddr_in);
            char ip_str[64];
            while ((fd = accept(m_socket, (sockaddr *) &peer_addr, &addr_len)) != INVALID_SOCKET) {
                uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
                uint16_t port = ntohs(peer_addr.sin_port);
                snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF,
                         ip & 0xFF);
                //log("CsslSocket accepted socket=%d from %s:%d", fd, ip_str, port);
                CSslSocket *pSocket = new CSslSocket(m_ssl_ctx, fd);
                pSocket->SetCallback(m_callback);
                pSocket->SetCallbackData(m_callback_data);
                pSocket->SetState(SOCKET_STATE_WAIT_HANDSHAKE);
                pSocket->SetRemoteIP(ip_str);
                pSocket->SetRemotePort(port);
                pSocket->setTcpNoDelay(true);
                pSocket->setNonblock(true);

                pSocket->setSoLinger(false, 0);
                pSocket->setReuseAddr(true);
                pSocket->setIntOption(SO_KEEPALIVE, 1);
                //pSocket->SetSendBufSize(640000);
                //pSocket->SetRecvBufSize(640000);

                AddBaseSocket(pSocket);
                CEventDispatch::Instance()->AddEvent(*pSocket, SOCKET_READ);

                //DEBUG("###Add timer %p %d",&pSocket->m_handshakeTimeout,pSocket->m_handshakeTimeout.interval);
                netlib_register_timer(&pSocket->m_handshakeTimeout);
                //CEventDispatch::Instance()->AddTimer(handshakeTimeout, pSocket, MAX_SSL_HANDSHAKE_DURATION);
                //m_callback(m_callback_data, NETLIB_MSG_CONNECT, (net_handle_t)fd, NULL);
            }
        }
            break;
        case SOCKET_STATE_WAIT_HANDSHAKE: {
            char buf[1];
            int n;
            n = recv(m_socket, buf, 1, MSG_PEEK);
            if (n == -1) {
                if (errno != EAGAIN) {
                    LogError("recv() failed");
                    Close();
                    return;
                }
            } else if (n == 1) {
                if (buf[0] & 0x80 || buf[0] == 0x16) {
                    m_ssl = ::SSL_new(m_ssl_ctx);
                    if (m_ssl == 0) {
                        logSslError("SSL_new");
                        cleanup();
                        return;
                    }
                    if (::SSL_set_fd(m_ssl, m_socket) == 0) {
                        logSslError("SSL_set_fd");
                        cleanup();
                        return;
                    }
                    ::SSL_set_accept_state(m_ssl);
                    if (::SSL_set_ex_data(m_ssl, s_ssl_connection_index, this) == 0) {
                        logSslError("SSL_set_ex_data");
                        cleanup();
                        return;
                    }
                    int rc = doHandshake();
                    if (rc == EAGAIN) {
                        SetState(SOCKET_STATE_HANDSHAKING);
                    } else if (m_isHandshaked) {
                        onSslHandshaked();
                    } else {
                        cleanup();
                    }
                } else {
                    LogError("received unknown {}", buf[0]);
                    cleanup();
                }
            } else {
                LogError("client closed connection");
                cleanup();
            }
        }
            break;
        case SOCKET_STATE_HANDSHAKING: {
            int rc = doHandshake();
            if (m_isHandshaked) {
                onSslHandshaked();
            } else if (rc != EAGAIN) {
                cleanup();
            }
        }
            break;
        default:
            if (m_io_state == SSL_IO_WANT_READ || m_io_state == SSL_IO_WANT_READ_STACKED) {
                m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t) m_socket, NULL);
            } else {
                m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t) m_socket, NULL);
            }
            break;
    }
}

void CSslSocket::OnWrite() {
#if defined(_WIN32) || defined(__APPLE__)
    CEventDispatch::Instance()->RemoveEvent(*this, SOCKET_WRITE);
#endif
    if (m_state == SOCKET_STATE_HANDSHAKING) {
        int rc = doHandshake();
        if (m_isHandshaked) {
            onSslHandshaked();
        } else if (rc != EAGAIN) {
            cleanup();
        }
    } else if (m_state == SOCKET_STATE_CONNECTING) {
        int error = 0;
        socklen_t len = sizeof(error);
#ifdef _WIN32
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
#else
        getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (void *) &error, &len);
#endif
        if (error) {
            LogError("getsockopt failed, err_code={}, {}", _GetErrorCode(), strerror(errno));
            m_callback(m_callback_data, NETLIB_MSG_CLOSE, (net_handle_t) m_socket, NULL);
        } else {
            m_state = SOCKET_STATE_CONNECTED;
            m_callback(m_callback_data, NETLIB_MSG_CONFIRM, (net_handle_t) m_socket, NULL);
        }
    } else {
        if (m_io_state == SSL_IO_WANT_WRITE || m_io_state == SSL_IO_WANT_WRITE_STACKED) {
            m_callback(m_callback_data, NETLIB_MSG_READ, (net_handle_t) m_socket, NULL);
        } else {
            m_callback(m_callback_data, NETLIB_MSG_WRITE, (net_handle_t) m_socket, NULL);
        }
    }
}

void CSslSocket::OnClose() {
    CBaseSocket::OnClose();
}
