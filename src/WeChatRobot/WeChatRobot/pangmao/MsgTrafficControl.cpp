/** @file MsgTrafficControl.cpp
  * @brief
  *		v0.1 �������㷨
  *		v0.2 ���������㷨����������https://www.cnblogs.com/linjiqin/p/9707713.html
  * @author yingchun.xu
  * @date 2020/8/7
  */

#include "stdafx.h"
#include "MsgTrafficControl.h"
#include "base/ZLogger.h"

#include <math.h>

const int kLimitMax = 5;
const int kLimitIntervalMax = 10 * 1000;

int g_traffic_limit_ = 2;		   // 2 ����Ϣ
int g_traffic_interval_ = 3 ;   // 3 s

std::atomic<int> g_count_ = 0;
std::atomic<time_t> g_last_limit_time_ = 0;
std::atomic<bool> g_can_i_need_reply_ = false;

std::string g_limit_answer_[] = {"æ������������������Ϣһ��������~", "è��ǰ��æ~", "��Ϣһ�£����ϻ���~"};

void CMsgTrafficControl::SetLimit(int interval, int limit) {
    if (limit > kLimitMax) {
        limit = kLimitMax;
    }

    if (interval > kLimitIntervalMax) {
        interval = kLimitIntervalMax;
    }

    g_traffic_limit_ = limit;
    g_traffic_interval_ = interval;

    LogInfo("�������������������������������{}s,������{}", interval, limit);
}

bool CMsgTrafficControl::Grant(bool& out_can_i_need_reply, std::string& out_random_answer) {
    time_t cur_time = time(nullptr);
    out_can_i_need_reply = false;
    // �����������㷨
    // ԭ��
    //      1.һ��ʼ����1����ʱ�������������+1�����counter>100���Һ͵�һ������ʱ�����1�����ڣ��򴥷�������
    //	    2.���ʱ������1���ӣ���counter����������Χ�ڣ�������counter
    //		PS������ǵ�һ�α����ƣ��򷵻�һ�������ʾ��
    // �ŵ㣺��
    // ȱ�㣺�ٽ����⣬2��ʱ�䴰���ڣ������յ�200�����󡣿���ʹ��TCP�Ļ������ڻ�����ƽ����
    // FIX ME��Ӧ����ԻỰ��������ʾ���������ĳЩȺ�ܻ�Ծ��������è�����������ƺ󣬵������ͻ�����޷�Ӧ�����⡣

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

