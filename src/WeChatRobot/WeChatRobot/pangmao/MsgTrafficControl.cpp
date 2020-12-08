/** @file MsgTrafficControl.cpp
  * @brief
  *		v0.1 计数器算法
  *		v0.2 滑动窗口算法限流，见：https://www.cnblogs.com/linjiqin/p/9707713.html
  * @author yingchun.xu
  * @date 2020/8/7
  */

#include "stdafx.h"
#include "MsgTrafficControl.h"
#include "base/ZLogger.h"

#include <math.h>

const int kLimitMax = 5;
const int kLimitIntervalMax = 10 * 1000;

int g_traffic_limit_ = 2;		   // 2 条消息
int g_traffic_interval_ = 3 ;   // 3 s

std::atomic<int> g_count_ = 0;
std::atomic<time_t> g_last_limit_time_ = 0;
std::atomic<bool> g_can_i_need_reply_ = false;

std::string g_limit_answer_[] = {"忙不过来了啦，请您休息一下再问呢~", "猫当前正忙~", "休息一下，马上回来~"};

void CMsgTrafficControl::SetLimit(int interval, int limit) {
    if (limit > kLimitMax) {
        limit = kLimitMax;
    }

    if (interval > kLimitIntervalMax) {
        interval = kLimitIntervalMax;
    }

    g_traffic_limit_ = limit;
    g_traffic_interval_ = interval;

    LogInfo("限流启动（计数器限流法），间隔：{}s,条数：{}", interval, limit);
}

bool CMsgTrafficControl::Grant(bool& out_can_i_need_reply, std::string& out_random_answer) {
    time_t cur_time = time(nullptr);
    out_can_i_need_reply = false;
    // 计数器限流算法
    // 原理：
    //      1.一开始设置1个计时器，处理请求后+1，如果counter>100，且和第一个请求时间差在1分钟内，则触发限流。
    //	    2.如果时间差大于1分钟，且counter还在限流范围内，则重置counter
    //		PS：如果是第一次被限制，则返回一条随机提示。
    // 优点：简单
    // 缺点：临界问题，2个时间窗口内，集中收到200条请求。可以使用TCP的滑动窗口机制来平滑。
    // FIX ME：应该针对会话级别做提示。否则如果某些群很活跃，导致胖猫进入限流机制后，单聊它就会出现无反应的问题。

    if (abs(cur_time - g_last_limit_time_) < g_traffic_interval_) {
        g_count_++;

        if (g_count_ >= g_traffic_limit_) {

            if (!g_can_i_need_reply_) {
                out_can_i_need_reply = g_can_i_need_reply_ = true;

                int index = rand() % (sizeof(g_limit_answer_) / sizeof(g_limit_answer_[0]));// sizeof(g_limit_answer_);
                out_random_answer = g_limit_answer_[index];
            }

            LogWarn("msg limited!cur={},max={}", g_count_, g_traffic_limit_);

            return false;
        }

    } else {
        g_count_ = 0;
        g_last_limit_time_ = cur_time;
        g_can_i_need_reply_ = false;
    }

    return true;
}

void TestIncreAndCheck() {
    //
    bool first_limit = false;

    for (int i = 0; i < 100; i++) {
        bool pass = false;

        while (!pass) {
            std::string answer = "";
            pass = CMsgTrafficControl::Grant(first_limit, answer);
            LogWarn("cur={},pass:{},first_limit:{},answer:{}", i, pass, first_limit, answer);

            if (!pass) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // 10 msg per second
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

