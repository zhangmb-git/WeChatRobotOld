//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include "reply.hpp"
#include "../protocol/common_def.h"


request_handler::request_handler() {
}

void request_handler::handle_request(char* buf) {
    int offset = sizeof(IMHeader);
    IMHeader* pHead = (IMHeader*)buf;
    void* data = buf + offset;

    int commandID = pHead->command_id;
    int  len = pHead->len;

    switch (commandID) {
    case kCmdTypeMsgData: {
        onSendTextMsg(commandID, len, data);
    }
    break;

    case kCmdTypeGetUserNickNameReq: {
        onGetUserNickNameMsg(commandID, len, data);
    }
    break;

    default:
        break;
    }

    return;
}


void  request_handler::onSendTextMsg(int32_t  cmdID, int32_t len, void* buf) {
    return;
}

void  request_handler::onGetUserNickNameMsg(int32_t  cmdID, int32_t len, void* buf) {
    return;
}



