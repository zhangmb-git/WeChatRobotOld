#pragma once

#include <vector>
#include <deque>
#include <mutex>

template<typename T>
class  CSafeQueue {
  public:
    CSafeQueue() {};
    ~CSafeQueue() {};

    void  PushBack(const T& value ) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push_back(value);
    }

    const T&  PopFront() {
        std::lock_guard<std::mutex> lk(m_mutex);
        T& value = m_queue.front();
        m_queue.pop_back();
        return value;
    }

    bool  IsEmpty() {
        return  (m_queue.size() <= 0);
    }
  private:
    std::mutex m_mutex;
    std::deque<T> m_queue;
};
