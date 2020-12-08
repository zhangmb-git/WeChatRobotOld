/** @file HttpConn.h
  * @brief HttpConn
  * @author yingchun.xu
  * @date 2020/8/4
  */
#ifndef __HTTP_CONN_H__
#define __HTTP_CONN_H__

#include <unordered_map>
#include "base/netlib.h"
#include "base/util.h"
#include "common/HttpParserWrapper.h"
#include "common/UtilPdu.h"


#define HTTP_CONN_TIMEOUT			60000

#define READ_BUF_SIZE	2048
#define HTTP_RESPONSE_HTML          "HTTP/1.1 200 OK\r\n"\
                                    "Connection:close\r\n"\
                                    "Content-Length:%d\r\n"\
                                    "Content-Type:text/html;charset=utf-8\r\n\r\n%s"

#define HTTP_RESPONSE_JSON          "HTTP/1.1 200 OK\r\n"\
                                    "Connection:close\r\n"\
                                    "Content-Length:%d\r\n"\
                                    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"

#define HTTP_RESPONSE_JSON_HEAD     "HTTP/1.1 200 OK\r\n"\
                                    "Connection:close\r\n"
//                                    "Content-Length:%d\r\n"\
//                                    "Content-Type:application/json;charset=utf-8\r\n\r\n%s"

#define HTTP_RESPONSE_HTML_MAX      1*1024*1024

enum {
    CONN_STATE_IDLE,
    CONN_STATE_CONNECTED,
    CONN_STATE_OPEN,
    CONN_STATE_CLOSED,
};

enum {
    METHOD_FRIENDLIST,  //�����б�
    METHOD_SENDINFO,    //������Ϣ
    METHOD_QUERYINFO,    //������Ϣ

};

enum {
    HTTP_RETURN_OK = 0,
    HTTP_RETURN_BODY_EMPTY = 1,
    HTTP_RETURN_PARSE_BODY_ERR = 2,

};

class CHttpConn {
  public:
    CHttpConn();
    virtual ~CHttpConn();

    uint32_t GetConnHandle() {
        return m_conn_handle;
    }
    char* GetPeerIP() {
        return (char*)m_peer_ip.c_str();
    }

    int Send(void* data, int len);
    void SendError(int code, string msg);
    void SendJson(string jsonContent);

    void Close();
    void OnConnect(net_handle_t handle);
    void OnRead();
    void OnWrite();
    void OnClose();
    void OnTimer(uint64_t curr_tick);
    void OnWriteComplete();

  private:
    bool _HandleRequest(std::string url, std::string bodyData);

    unsigned char _fromHex(unsigned char x);
    std::string _urlDecode(const std::string& str);

  protected:
    net_handle_t	m_sock_handle;
    uint32_t		m_conn_handle;
    bool			m_busy;

    uint32_t        m_state;
    std::string		m_peer_ip;
    uint16_t		m_peer_port;
    CSimpleBuffer	m_in_buf;
    CSimpleBuffer	m_out_buf;

    uint64_t		m_last_send_tick;
    uint64_t		m_last_recv_tick;

    CHttpParserWrapper m_cHttpParser;
};

typedef std::unordered_map<uint32_t, CHttpConn*> HttpConnMap_t;

CHttpConn* FindHttpConnByHandle(uint32_t handle);
void init_http_conn();

void _sendJson(uint32_t handle, string jsonContent);

// user

void onFriendGetAccountInfo(uint32_t handle, const std::string& url, const std::string& body);
void onFriendFreshQrCode(uint32_t handle, const std::string& url, const std::string& body);

// msg
void onSendMsg(uint32_t handle, const std::string& url, const std::string& body);
void _onSendFileMsg(int msg_type, const string wxid, const string url);
void onSendAtAllMsg(uint32_t handle, const std::string& url, const std::string& body);
void onRegisterReceiveMsgCallback(uint32_t handle, const std::string& url, const std::string& body);

// friend
void onGetFriendList(uint32_t handle, const std::string& url, const std::string& body);
void onFriendAdd(uint32_t handle, const std::string& url, const std::string& body);
void onFriendDel(uint32_t handle, const std::string& url, const std::string& body);

// group
void onGroupChangeName(uint32_t handle, const std::string& url, const std::string& body);
void onGroupGetList(uint32_t handle, const std::string& url, const std::string& body);

// group member
void onGroupMemberList(uint32_t handle, const std::string& url, const std::string& body);
void onGroupMemberAdd(uint32_t handle, const std::string& url, const std::string& body);
void onGroupMemberDel(uint32_t handle, const std::string& url, const std::string& body);

#endif /* IMCONN_H_ */
