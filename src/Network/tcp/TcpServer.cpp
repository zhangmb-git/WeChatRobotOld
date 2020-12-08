#include "TcpServer.h"
#include <asio.hpp>


std::shared_ptr<TcpServer> TcpServer::instance_ = std::make_shared<TcpServer>();

using asio::ip::tcp;

TcpServer::TcpServer()	: m_context(),
    m_acceptor(m_context, tcp::endpoint(tcp::v4(), 8201)),
    m_socket_(m_context),
    connection_manager_(), request_handler_() {

    // m_thread = std::move(std::thread(std::bind(&TcpServer::run, this)));

    /* tcp::resolver resolver(m_context);
     tcp::resolver::query  query("0.0.0.0", "8081");
     tcp::endpoint endpoint = *resolver.resolve(query);*/


    //do_accept();

    m_context.run();


}



TcpServer::~TcpServer() {
    stop();
}


std::shared_ptr<TcpServer> TcpServer::getInstance() {
    return instance_;
}

void TcpServer::start() {

    do_accept();
}

void TcpServer::run() {
    m_context.run();
}

void TcpServer::stop() {
    m_context.stop();

    if (m_thread.joinable()) {
        m_thread.join();
    }

}

//
//void TcpServer::start_accept() {
//    tcp_connection::pointer new_connection = tcp_connection::create(*m_context.get());
//
//    m_acceptor->async_accept(new_connection->socket(),
//                             std::bind(&TcpServer::handle_accept, this, new_connection, std::placeholders::_1));
//    return;
//}
//
//
//void TcpServer::handle_accept(tcp_connection::pointer new_connection,
//                              const asio::error_code& error) {
//    if (!error) {
//        new_connection->start();
//    }
//
//    start_accept();
//    return;
//}

void  TcpServer::do_accept() {


    m_acceptor.async_accept(m_socket_,
    [ this](asio::error_code ec)  {
        printf("accept====================================");

        if (!m_acceptor.is_open()) {
            return;
        }

        printf("accept====================================done");

        if (!ec) {
            connection_manager_.start(std::make_shared<connection>(m_socket_, connection_manager_, request_handler_));
        }

        do_accept();
    });

}

void TcpServer::do_await_stop() {
    //signals_.async_wait(
    //[this](asio::error_code /*ec*/, int /*signo*/) {
    //    // The server is stopped by cancelling all outstanding asynchronous
    //    // operations. Once all operations have finished the io_service::run()
    //    // call will exit.
    //    acceptor_.close();
    //    connection_manager_.stop_all();
    //});
}