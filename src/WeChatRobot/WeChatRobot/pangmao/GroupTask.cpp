#include "stdafx.h"
#include "GroupTask.h"
#include "MsgHelper.h"
#include "base/ZLogger.h"
#include "base/util.h"
#include "api/MsgManager.h"


//group task==============================
void  CGroupTask::onExecute() {
    if (m_HttpCallback) {
        m_HttpCallback();
    }

    return  ;
}

//
void  CGroupTask::SetHttpCallBack(const HttpCallBack_Func& func) {
    m_HttpCallback = std::move(func);
}

void  CGroupTask::SetTaskParam(const CString& strGroupID) {
    m_strGroupID = strGroupID;
    return;
}

void  CGroupTask::SendGroupMemberRequest(const CString& strGroupID) {
    CMsgManager::GetInstance()->SendGetGroupMemberListMsg(strGroupID);
    return;
}

CGroupTaskMgr* CGroupTaskMgr::s_pTaskMgr = nullptr;

CGroupTaskMgr* CGroupTaskMgr::getInstance() {
    if (s_pTaskMgr == nullptr) {
        s_pTaskMgr = new CGroupTaskMgr();
    }

    return  s_pTaskMgr;
}


CGroupTaskMgr::CGroupTaskMgr() {
    m_groupQueueService.start();

}

CGroupTaskMgr::~CGroupTaskMgr() {
    m_groupQueueService.stop();
}



void  CGroupTaskMgr::AddTask(const std::string& strID, CGroupTask* task) {

    m_mapGroupTask[strID].push_back(task);
    return;
}

void  CGroupTaskMgr::ActivateTask(const std::string& strTaskID) {
    if (m_mapGroupTask.find(strTaskID) == m_mapGroupTask.end()) {
        LogError("activiate task  error");
        return;
    }

    for (auto& iter : m_mapGroupTask[strTaskID]) {
        m_groupQueueService.addTask(iter);
    }

    m_mapGroupTask.erase(strTaskID);
}

