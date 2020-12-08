/** @file MsgAutoForward.h
  * @brief 资讯转发
  * @author yingchun.xu
  * @date 2020/8/5
  */

#ifndef _MSGAUTOFORWARD_839AE062_FEB6_4B13_9CD6_6AD12E8C4984_
#define _MSGAUTOFORWARD_839AE062_FEB6_4B13_9CD6_6AD12E8C4984_

#include "common/public_define.h"
#include "base/thread_pool.h"

#include <unordered_set>
#include <unordered_set>
#include <atomic>
#include <memory>

using std::unordered_set;
using std::string;
using std::atomic;
using std::unique_ptr;

class CMsgAutoForward {
  public:
    // 获取单例
    static CMsgAutoForward* GetInstance();
    // 启动资讯转发
    void StartAutoForward();


  private:
    CMsgAutoForward();
    void onReceiveMsg(const Message& msg);

    void onForwardText(const Message& msg);
    void onForwardImage(const Message& msg);
    void onBoradcastAsync(const CString& fromId, const int& msg_type, const CString& content);

  private:
    static CMsgAutoForward* instance_;
    unordered_set<string> set_to_group_;
    unordered_set<string> set_listen_;
    unique_ptr<CThreadPool> thread_pool_;

    bool is_forward_image_; // 是否转发图片
    bool is_forward_;       // 转发开关
    atomic<bool> is_init_;
    atomic<uint32_t> task_id_;

    int64_t last_send_time_;
    int     forward_interval_; // 资讯转发间隔限制
};

#endif//_MSGAUTOFORWARD_839AE062_FEB6_4B13_9CD6_6AD12E8C4984_

