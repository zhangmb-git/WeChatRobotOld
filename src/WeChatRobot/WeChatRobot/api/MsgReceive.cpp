#include "stdafx.h"

#include "MsgReceive.h"
#include <chrono>
#include "base/ZLogger.h"
#include "base/util.h"

CMsgReceive* CMsgReceive::instance_ = new CMsgReceive();

CMsgReceive* CMsgReceive::GetInstance() {
    return instance_;
}

CMsgReceive::CMsgReceive(): thread_run_(true) {
    thread msgConsumerThread_(&CMsgReceive::_ConsumerMsg, this);
    msgConsumerThread_.detach();
}

CMsgReceive::~CMsgReceive() {
    thread_run_ = false;
    cv_.notify_all();
}

void CMsgReceive::OnHandleMsg(const Message& msg) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        msgs_.push_back(msg);
    }
    cv_.notify_all();
}

void CMsgReceive::Register(const ReciveMessageCallback& callback, const string& key) {
    callbacks_[key] = std::move(callback);
}

void CMsgReceive::Unregister(const string& key) {
    callbacks_.erase(key);
}

void CMsgReceive::_ConsumerMsg() {
    LogDebug("consumer thread start");

    while (thread_run_) {
        // LogDebug("wait produce msg...");
        unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk);
        lk.unlock();

        if (!thread_run_) {
            LogDebug("consumer thread exit...");
            break;
        }

        while (true) {
            if (mutex_.try_lock() == true) {
                if (!msgs_.empty()) {
                    const Message& msg = msgs_.front();
                    msgs_.pop_front();
                    mutex_.unlock();

                    LogDebug("consume one msg,from:{},to:{},content:{}", wstring2string(msg.msgSender), wstring2string(msg.source), wstring2string(msg.content));

                    // »Øµ÷
                    for (auto& item : callbacks_) {
                        (item.second)(msg);
                    }

                } else {
                    mutex_.unlock();
                    break;
                }

            } else {
                std::this_thread::sleep_for(1ms);
            }
        }
    }
}
