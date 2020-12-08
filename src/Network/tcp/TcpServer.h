#ifndef _TCPSERVER_C490E426_DC3A_4C14_9E29_7A124BD603A5_
#define _TCPSERVER_C490E426_DC3A_4C14_9E29_7A124BD603A5_

#include <memory>
#include <cstdint>
#include <asio.hpp>
#include <thread>
#include <functional>
#include "protocol/common_def.h"
#include "connection.hpp"
#include "connection_manage.hpp"


//using asio::ip::tcp;

// 接口回调
class ITcpServer {
  public:
    virtual void onHandle(IMHeader& header, uint8_t* data, uint32_t len) = 0;
};

// TCP Server实现类
class TcpServer {
  private:
    static std::shared_ptr<TcpServer> instance_;

  public:
    TcpServer();
    //TcpServer(asio::io_context& context);
    ~TcpServer();
    static std::shared_ptr<TcpServer> getInstance();

  public:
    void start();
    void run();
    void stop();

    asio::io_context&  getContext() {
        return  m_context;
    }


  private:
    //void  start_accept();
    //void  handle_accept(connection::pointer new_connection, const asio::error_code& error);
    void  do_accept();
    void  do_await_stop();


  private:

    asio::ip::tcp::socket m_socket_;
    asio::io_context  m_context;
    tcp::acceptor  m_acceptor;
    connection_manager connection_manager_;
    request_handler  request_handler_;
    std::thread  m_thread;

};


#endif//_TCPSERVER_C490E426_DC3A_4C14_9E29_7A124BD603A5_