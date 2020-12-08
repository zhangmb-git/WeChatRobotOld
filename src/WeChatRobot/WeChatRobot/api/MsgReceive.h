/** @file CMsgReceive.h
  * @brief 消息接收管理中心
  * @author yingchun.xu
  * @date 2020/8/4
  */

#ifndef _CMSGRECEIVE_3C31987A_B9B3_458D_9101_9B21CF07ACD6_
#define _CMSGRECEIVE_3C31987A_B9B3_458D_9101_9B21CF07ACD6_

#include <memory>
#include <functional>
#include <unordered_map>
#include <deque>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "common/public_define.h"

// boost-thread
//#include "boost/thread/condition.hpp"

using namespace std;

class CMsgReceive {
  public:
    typedef function<void(const Message&)> ReciveMessageCallback;

    // 获取单例
    static CMsgReceive* GetInstance();

    // 收到新消息回调
    void OnHandleMsg(const Message& msg);

    // 注册回调
    void Register(const ReciveMessageCallback& callback, const string& key);
    void Unregister(const string& key);

  private:
    void _ConsumerMsg();

  private:
    static CMsgReceive* instance_;

    CMsgReceive();
    // delete
    CMsgReceive(const CMsgReceive&) = delete;
    CMsgReceive& operator=(const CMsgReceive&) = delete;
    ~CMsgReceive();

    unordered_map<string, ReciveMessageCallback> callbacks_;
    deque<Message> msgs_;

    thread msgConsumerThread_;
    bool thread_run_;

    //boost::condition_variable cond_;
    //boost::mutex mutex_;

    std::condition_variable cv_;
    std::mutex mutex_;
};

#endif//_CMSGRECEIVE_3C31987A_B9B3_458D_9101_9B21CF07ACD6_


