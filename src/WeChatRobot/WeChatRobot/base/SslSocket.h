#ifndef __SSL_SOCKET_H__
#define __SSL_SOCKET_H__

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <string>

#include "ostype.h"
#include "BaseSocket.h"
#include "netlib.h"

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")


enum {
	SSL_IO_NORMAL = 0,
	SSL_IO_WANT_READ,
	SSL_IO_WANT_READ_STACKED,
	SSL_IO_WANT_WRITE,
	SSL_IO_WANT_WRITE_STACKED,
};

class CSslSocket : public CBaseSocket
{
public:
	CSslSocket(const char *cert_file, const char *key_file, const char *dh_file);
	CSslSocket(SSL_CTX *ctx, SOCKET sock);
	virtual ~CSslSocket();

	bool isHandshaked() const { return m_isHandshaked; }
        virtual int Listen(
		const char*		server_ip, 
		uint16_t		port,
		callback_t		callback,
		void*			callback_data);

	virtual net_handle_t Connect(
		const char*		server_ip, 
		uint16_t		port,
		callback_t		callback,
		void*			callback_data);

	virtual int Send(void* buf, int len);

	virtual int Recv(void* buf, int len);

	virtual int Close();

	virtual void OnRead();
	virtual void OnWrite();
	virtual void OnClose();

private:
	void cleanup();
	int doHandshake();
	void onSslHandshaked();

	static int s_ssl_initialized;
	static int s_ssl_connection_index;
	rb_timer_item m_handshakeTimeout;
	static void sslInfoCallback(const SSL *ssl, int where, int ret);
	static void logSslError(const char* op);
	static void handshakeTimeout(void* data, uint8_t msg, uint32_t handle, void* pParam);

	struct ScopedBio
	{
		BIO* p;
		~ScopedBio() { if (p) ::BIO_free(p); }
	};

	struct ScopedDh
	{
		DH* p;
		~ScopedDh() { if (p) ::DH_free(p); }
	};

	SSL_CTX* _initSslContext();
protected:
	std::string			m_cert;
	std::string			m_key;
	std::string			m_dh;
	SSL_CTX			*m_ssl_ctx;
	SSL			*m_ssl;
	ScopedBio		m_bio;
	bool			m_ownCtx;
	bool			m_isHandshaked;
	bool			m_renegotiation;
	int			    m_io_state;
};

void AddBaseSocket(CBaseSocket *pSocket);

#endif
