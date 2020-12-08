//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma  once
#include <string>



struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler {
  public:
    request_handler(const request_handler&) = delete;
    request_handler& operator=(const request_handler&) = delete;

    /// Construct with a directory containing files to be served.
    explicit request_handler();

    /// Handle a request and produce a reply.
    //void handle_request(const request& req, reply& rep);
    //void handle_request(int32_t fromID, int32_t  cmdID, int32_t len, void* buf);
    void handle_request(char* buf);

  private:
    void  onSendTextMsg(int32_t  cmdID, int32_t len, void* buf);
    void  onGetUserNickNameMsg(int32_t  cmdID, int32_t len, void* buf);

};



