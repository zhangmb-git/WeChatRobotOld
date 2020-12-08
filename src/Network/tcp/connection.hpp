//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma  once
#include <array>
#include <memory>
#include <asio.hpp>
#include "connection_manage.hpp"
#include "request_handler.hpp"
#include "reply.hpp"


using asio::ip::tcp;
class connection_manager;

/// Represents a single connection from a client.
class connection  : public std::enable_shared_from_this<connection> {
  public:
    connection(const connection&) = delete;
    connection& operator=(const connection&) = delete;

    /// Construct a connection with the given socket.
    explicit connection(asio::ip::tcp::socket& socket,
                        connection_manager& manager, request_handler& handler);

    /// Start the first asynchronous operation for the connection.
    void start();

    /// Stop all asynchronous operations associated with the connection.
    void stop();

  private:
    /// Perform an asynchronous read operation.
    void do_read();

    /// Perform an asynchronous write operation.
    void do_write();

    //asio::io_context  io_service_;
    /// Socket for the connection.
    asio::ip::tcp::socket socket_;

    /// The manager for this connection.
    connection_manager& connection_manager_;

    /// The handler used to process the incoming request.
    request_handler& request_handler_;

    /// Buffer for incoming data.
    std::array<char, 8192> buffer_;

    reply reply_;



};

typedef std::shared_ptr< connection > connection_ptr;



