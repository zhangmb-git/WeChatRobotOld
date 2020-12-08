#pragma once
#include <memory>
#include <list>
#include <deque>
#include <thread>
#include <map>
#include <unordered_map>
#include "base/common_thread.hpp"
#include "common/public_define.h"

typedef  std::function<void(const tagNickNameParam& nickNameParam)>  CallBack_NickNameTask;
class  CNickNameTask : public CTask {
  public:
    virtual void onExecute();

    void  SetTaskParam(const tagNickNameParam&  param);
    void  SetCallBack(const CallBack_NickNameTask& func);
    void  SetNickNameParam(const std::string&  strNickName);


  private:
    CallBack_NickNameTask   m_func;
    tagNickNameParam  m_param;

};


//
class CNickNameTaskMgr {
  public:
    static CNickNameTaskMgr* getInstance();
    ~CNickNameTaskMgr();


    void  addTask(const std::string& strWxGroupID, const std::string& strWxID, const tagHttpResp& tHttpResp);
    //void  handleTask(const std::string& userID, const std::string& nickName);
    void  ActivateTask(const std::string& strGroupID, const std::string& strTaskID);
  private:
    void  _GetGroupMemberInfo(CString wxGroupID, CString wxUserID);

  private:
    static  CNickNameTaskMgr*  s_pTaskMgr;
    CNickNameTaskMgr();
  private:

    std::mutex  m_mapMutex;
    std::unordered_map<std::string, std::vector<CNickNameTask*> > m_mapNickNameTask;

    CNotifyQueueService  m_NotifyQueueService;

};

