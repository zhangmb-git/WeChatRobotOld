//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma  once
#include <set>
#include "connection.hpp"

class connection;
typedef std::shared_ptr< connection > connection_ptr;

class connection_manager {
  public:
    connection_manager(const connection_manager&) = delete;
    connection_manager& operator=(const connection_manager&) = delete;

    /// Construct a connection manager.
    connection_manager();

    /// Add the specified connection to the manager and start it.
    void start(connection_ptr c);

    /// Stop the specified connection.
    void stop(connection_ptr c);

    /// Stop all connections.
    void stop_all();

  private:
    /// The managed connections.
    std::set<connection_ptr> connections_;
};


