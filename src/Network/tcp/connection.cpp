//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <utility>
#include <vector>
#include "connection_manage.hpp"
#include "request_handler.hpp"



connection::connection(asio::ip::tcp::socket& socket,
                       connection_manager& manager, request_handler& handler)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      request_handler_(handler) {

}

void connection::start() {
    do_read();
}

void connection::stop() {
    socket_.close();
}

void connection::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(buffer_),
    [this, self](asio::error_code ec, std::size_t bytes_transferred) {
        if (!ec) {
            printf("hello=============================================");
            //request_handler_.handle_request(buffer_.data());
            do_write();
            //do_read();


        } else if (ec != asio::error::operation_aborted) {
            connection_manager_.stop(shared_from_this());
        }
    });
}

void connection::do_write() {
    auto self(shared_from_this());

    asio::async_write(socket_, reply_.to_buffers(),
    [this, self](asio::error_code ec, std::size_t len) {
        if (!ec) {
            // Initiate graceful connection closure.
            asio::error_code ignored_ec;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
        }

        if (ec != asio::error::operation_aborted) {
            connection_manager_.stop(shared_from_this());
        }
    });
}
