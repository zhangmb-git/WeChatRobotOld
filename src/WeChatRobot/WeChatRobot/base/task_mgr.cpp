#include "task_mgr.h"


CTaskMgr* CTaskMgr::s_pTaskMgr = nullptr;

CTaskMgr* CTaskMgr::getInstance() {
    if (s_pTaskMgr == nullptr) {
        s_pTaskMgr = new CTaskMgr();
    }

    return  s_pTaskMgr;
}


CTaskMgr::CTaskMgr() {
    Loop();
}

CTaskMgr::~CTaskMgr() {
    m_threadpool.stop();
}

void  CTaskMgr::AddTask(int taskid, tagTask* task) {
    m_threadpool.addTask(taskid, task);
}

void  CTaskMgr::AddTask(const std::string& strID, tagTask* task) {
    std::hash<std::string>  hash_str;
    int64_t hashID = hash_str(strID);
    m_threadpool.addTask(hashID, task);
    return;
}

void  CTaskMgr::Loop() {
    //2个线程应该够了
    m_threadpool.init(2);
    m_threadpool.start();
}

