/** @file HttpServer.h
  * @brief HttpServer
  * @author yingchun.xu
  * @date 2020/8/4
  */

#ifndef _HTTPSERVER_82D3C575_0672_45AD_B78A_881E2A672B5B_
#define _HTTPSERVER_82D3C575_0672_45AD_B78A_881E2A672B5B_

#include <string>

#include "base/thread_pool.h"
#include "base/EventDispatch.h"

//class  CHttpService : public  common_thread {
//  public:
//    CHttpService() {}
//    virtual  ~CHttpService() {}
//
//  private:
//    void    onExecute() {
//        CEventDispatch::Instance()->StartDispatch();
//    }
//};
class CHttpServer {
  public:
    CHttpServer(void);
    ~CHttpServer(void);
    bool Start();
    void Stop();

  public:
    void  execute();

  private:
    std::string  m_strIP;
    int   m_port;
    common_thread  m_loop;
};


#endif//_HTTPSERVER_82D3C575_0672_45AD_B78A_881E2A672B5B_