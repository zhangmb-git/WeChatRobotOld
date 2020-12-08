/** @file TcpClient.h
  * @brief TcpClient
  * @author yingchun.xu
  * @date 2020/8/14
  */


#ifndef _TCPCLIENT_434ACB0D_1E03_4A69_909C_163018E1D3DE_
#define _TCPCLIENT_434ACB0D_1E03_4A69_909C_163018E1D3DE_

#include <string>
#include "protocol/common_def.h"
#include <asio.hpp>
#include <functional>
#include <unordered_map>

using asio::ip::tcp;
using namespace std;

//class ITcpClient {
//  public:
//    virtual void onHandle(IMHeader& header, uint8_t* data, uint32_t len) = 0;
//};

typedef std::function<void(void* resp)> ResponseCb;

struct Request {
    IMHeader header;
    ResponseCb cb;
};

//template <typename T>
//class Response {
//
//};

class TcpClient {
  public:
    TcpClient(/*asio::io_context& io_context*/);
    ~TcpClient() = default;

  public:
    bool connect(std::string ip, uint16_t port);
    void close();

    // 发送请求，有响应
    bool sendReq(CommondType command_id, uint8_t* data, uint32_t len, const ResponseCb& cb);
    // 发送通知，无响应
    bool sendNotify(CommondType command_id, uint8_t* data, uint32_t len);

  protected:
    virtual void onHandle(IMHeader& header, uint8_t* data, uint32_t len) = 0;

  private:
    //std::weak_ptr<ITcpClient> callback_;
    bool is_connect_;

    asio::io_context& socket_io_context_;
    tcp::socket socket_;
    uint8_t* buffer_;

    unordered_map<uint16_t, Request> map_request_;
};

void init_tcp_client();

#endif//_TCPCLIENT_434ACB0D_1E03_4A69_909C_163018E1D3DE_