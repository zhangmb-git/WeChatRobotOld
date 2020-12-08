/*
 *  a wrap for non-block socket class for Windows, LINUX and MacOS X platform
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "ostype.h"
#include <string>

enum
{
	SOCKET_STATE_IDLE,
	SOCKET_STATE_LISTENING,
	SOCKET_STATE_CONNECTING,
	SOCKET_STATE_CONNECTED,
	SOCKET_STATE_CLOSING,
	SOCKET_STATE_WAIT_HANDSHAKE,
	SOCKET_STATE_HANDSHAKING,
	SOCKET_STATE_HANDSHAKED
};

class CBaseSocket //: public CRefObject
{
public:
	CBaseSocket();
	
	virtual ~CBaseSocket();

	SOCKET GetSocket() { return m_socket; }
	void SetSocket(SOCKET fd) { m_socket = fd; }
	void SetState(uint8_t state) { m_state = state; }

	void SetCallback(callback_t callback) { m_callback = callback; }
	void SetCallbackData(void* data) { m_callback_data = data; }
	void SetRemoteIP(char* ip) { m_remote_ip = ip; }
	void SetRemotePort(uint16_t port) { m_remote_port = port; }
	void SetSendBufSize(int send_size);
	void SetRecvBufSize(int recv_size);
	void setNonblock(bool noBlock);
	void setTcpNoDelay(bool noDelay);

	void setReuseAddr(bool on);
	bool setIntOption(int option, int value);
	bool setSoLinger(bool doLinger, int seconds);		

	bool getReadActive() const { return m_read_active; }
	bool getWriteActive() const { return m_write_active; }
	void setReadActive(bool active) { m_read_active = active; }
	void setWriteActive(bool active) { m_write_active = active; }

	const char*	GetRemoteIP() { return m_remote_ip.c_str(); }
	uint16_t	GetRemotePort() { return m_remote_port; }
	const char*	GetLocalIP() { return m_local_ip.c_str(); }
	uint16_t	GetLocalPort() { return m_local_port; }
public:
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

protected:	
	int _GetErrorCode();
	bool _IsBlock(int error_code);

	void _SetAddr(const char* ip, const uint16_t port, sockaddr_in* pAddr);

	int _AcceptNewSocket();
	
	bool _setTimeOption(int option, int sec, int usec);	
	bool _checkSocketHandle();

	std::string			m_remote_ip;
	uint16_t		m_remote_port;
	std::string			m_local_ip;
	uint16_t		m_local_port;

	callback_t		m_callback;
	void*			m_callback_data;

	uint8_t			m_state;
	SOCKET			m_socket;
	bool			m_read_active;
	bool			m_write_active;
};

CBaseSocket* FindBaseSocket(net_handle_t fd);

#endif
