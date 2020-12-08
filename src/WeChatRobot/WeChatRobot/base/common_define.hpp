#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include <chrono>
#include <list>


typedef  std::function<void()>  execute_func;
typedef  std::function<void(void*)>  CallBack_Func_1;

struct tagTask {

    virtual bool  execute() {
        return true;
    }
    virtual void  onExecute() {};  //执行自己的回调
    virtual void  onSuccess() {};
    virtual void  onFail() {};
};

class CTask : public  tagTask {
  public:
    CTask() {
    }

    virtual ~CTask() {
        if (m_task_data_) {
            delete  m_task_data_;
            m_task_data_ = nullptr;
        }
    }
    virtual bool execute() override {

        onExecute();

        if (m_callback_) {
            m_callback_(m_task_data_);
        }

        return  true;
    }

    void  SetTaskParam(void* data) {

        m_task_data_ = std::move(data);
    }

    void  SetCallBack(const CallBack_Func_1& func) {
        m_callback_ = std::move(func);
    }
  protected:
    void*  m_task_data_;
  private:
    CallBack_Func_1  m_callback_;
};

