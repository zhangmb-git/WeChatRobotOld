#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include <chrono>
#include <list>
#include "common_define.hpp"



class common_thread {
  public:

    common_thread() {
        m_thread = std::thread(&common_thread::run, this);
    };

    ~common_thread() {
        stop();
    };

    void set_exec_func(execute_func func) {
        m_exec = std::move(func);
    }

    void  start() {
        m_runFlag = true;
        m_cv.notify_one();
    }

    void  stop() {
        m_runFlag = false;
        m_cv.notify_one();

        if (m_thread.joinable()) {
            m_thread.join();
        }
    }



    void run() {
        //printf("thread enter run..............");
        {

            std::unique_lock<std::mutex> lck(m_cvMutex);
            m_cv.wait_for(lck, std::chrono::milliseconds(5000), [this] {return m_runFlag == true; });
        }
        //printf("thread start run..............");

        while (m_runFlag) {
            //printf("thread running normal.......");

            if (m_exec) {
                m_exec();

            } else {
                //printf("thread executable func null");
                m_runFlag = false;
                break;
            }

            //std::this_thread::sleep_for(std::chrono::milliseconds(10));

        }//while

        //printf("thread stop run ..............");
    }

  protected:
    execute_func  m_exec;

  private:
    bool  m_runFlag;
    std::mutex  m_cvMutex;
    std::thread  m_thread;
    std::condition_variable  m_cv;

};

//队列服务
class  CQueueService : public common_thread {
  public:
    CQueueService() {
        m_exec = std::bind(&CQueueService::execute, this);
    }

    virtual ~CQueueService() {};

    virtual void execute() {
        std::list<tagTask*>  temp_task_list;
        {
            std::lock_guard<std::mutex>  lock(m_mutex);
            temp_task_list.swap(_task_list);
        }

        while (!temp_task_list.empty()) {
            tagTask* pTask = temp_task_list.front();
            temp_task_list.pop_front();

            if (pTask) {
                bool bRet = pTask->execute();

                if (bRet) {
                    //printf("task execute success！\n");
                }

                delete  pTask;
                pTask = nullptr;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }

    void  addTask(tagTask* pTask) {
        std::lock_guard<std::mutex>  lock(m_mutex);
        _task_list.push_back(pTask);
    }

  private:
    std::mutex  m_mutex;
    std::list<tagTask*>  _task_list;

};

//队列服务
class  CNotifyQueueService : public common_thread {
  public:
    CNotifyQueueService() {
        m_exec = std::bind(&CNotifyQueueService::execute, this);
    }

    virtual ~CNotifyQueueService() {};

    virtual void execute() {

        {
            std::unique_lock<std::mutex> lk(m_cvMutex);
            m_cvList.wait_for(lk, std::chrono::seconds(1), [this] {return !_task_list.empty(); });
        }

        while (!_task_list.empty()) {

            tagTask* pTask = nullptr;

            {
                std::lock_guard <std::mutex>  lck(m_mutex);
                pTask = _task_list.front();
                _task_list.pop_front();
            }

            if (pTask) {
                bool bRet = pTask->execute();

                if (bRet) {
                    //printf("task execute success！\n");
                }

                delete  pTask;
                pTask = nullptr;
            }
        }
    }

    void  addTask(tagTask* pTask) {
        std::lock_guard<std::mutex>  lock(m_mutex);
        _task_list.push_back(pTask);
        m_cvList.notify_one();
    }

  private:
    std::mutex  m_mutex;
    std::mutex  m_cvMutex;
    std::condition_variable m_cvList;
    std::list<tagTask*>  _task_list;

};


