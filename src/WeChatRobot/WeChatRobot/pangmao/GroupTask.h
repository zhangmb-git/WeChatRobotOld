#pragma once
#include <memory>
#include <list>
#include <deque>
#include <thread>
#include <map>
#include <unordered_map>
#include "base/common_thread.hpp"
#include "common/public_define.h"

typedef  std::function<void()>  HttpCallBack_Func;

class CGroupTask : public  CTask {
  public:
    virtual void onExecute() override;
    void  SetTaskParam(const CString& strGroupID);
    void  SetHttpCallBack(const HttpCallBack_Func& func);
    void  SendGroupMemberRequest(const CString& strGroupID);

  private:
    CString m_strGroupID;
    HttpCallBack_Func  m_HttpCallback;
};


//
class CGroupTaskMgr {
  public:
    static CGroupTaskMgr* getInstance();
    ~CGroupTaskMgr();

    void  AddTask(const std::string& strTask, CGroupTask* task);
    void  ActivateTask(const std::string& strTask);

  private:
    static  CGroupTaskMgr*  s_pTaskMgr;
    CGroupTaskMgr();
  private:

    std::mutex  m_mapMutex;
    std::unordered_map<std::string, std::vector<CGroupTask*> > m_mapGroupTask;
    CNotifyQueueService  m_groupQueueService;

};

