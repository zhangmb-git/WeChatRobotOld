#include "stdafx.h"
#include <string>
#include "HttpServer.h"
#include "base/netlib.h"
#include "HttpConn.h"
#include <functional>
#include "CSysConfig.h"
#include "base/util.h"

#include "MsgPush.h"

void http_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam) {
    if (msg == NETLIB_MSG_CONNECT) {
        CHttpConn* pConn = new CHttpConn();
        pConn->OnConnect(handle);

    } else {
        printf("!!!error msg: %d ", msg);
    }
}

using namespace std;

CHttpServer::CHttpServer(void) {
    netlib_init();

    auto func = std::bind(&CHttpServer::execute, this);
    m_loop.set_exec_func(func);
    m_loop.start();
}

CHttpServer::~CHttpServer(void) {
    Stop();
    netlib_destroy();
}


void  CHttpServer::execute() {
    netlib_eventloop();
}

bool CHttpServer::Start() {
    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr || pConfig->serverIP.empty() || pConfig->port == 0) {
        return  false;
    }

    int ret = netlib_listen(pConfig->serverIP.c_str(), pConfig->port, http_callback, nullptr);

    if (ret == NETLIB_ERROR)
        return false;

    // 启动消息HTTP回调推送
    StartHttpMsgPush();

    return true;
}


void CHttpServer::Stop() {
    netlib_stop_event();
    m_loop.stop();
    return;
}