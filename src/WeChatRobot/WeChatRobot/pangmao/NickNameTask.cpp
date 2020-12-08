#include "stdafx.h"
#include "NickNameTask.h"
#include "MsgHelper.h"
#include "base/ZLogger.h"
#include "base/util.h"
#include "api/MsgManager.h"

//
void  CNickNameTask::onExecute() {
    if (m_func) {

        m_func(m_param);
        LogInfo("execute nickname task,param  type:{},respText:{},nickname:{},userID:{},groupID:{}", m_param.tHttpResp.respType,
                Wchar_tToString(StringToWchar_t(m_param.tHttpResp.respText)), m_param.strNickName, m_param.strWxUserID, m_param.strWxGroupID);
    }

    return;
}

void  CNickNameTask::SetTaskParam(const tagNickNameParam&  param) {
    m_param = param;
}

void  CNickNameTask::SetCallBack(const CallBack_NickNameTask& func) {
    m_func = std::move(func);
}

void  CNickNameTask::SetNickNameParam(const std::string&  strNickName) {
    m_param.strNickName = strNickName;
}



CNickNameTaskMgr* CNickNameTaskMgr::s_pTaskMgr = new CNickNameTaskMgr();

CNickNameTaskMgr* CNickNameTaskMgr::getInstance() {
    return  s_pTaskMgr;
}


CNickNameTaskMgr::CNickNameTaskMgr() {
    m_NotifyQueueService.start();

}

CNickNameTaskMgr::~CNickNameTaskMgr() {
    m_NotifyQueueService.stop();
}


//添加获取昵称任务
void  CNickNameTaskMgr::addTask(const std::string& strWxGroupID, const std::string& strWxUserID, const tagHttpResp& resp) {

    tagNickNameParam  tNickNameParam;
    tNickNameParam.tHttpResp = resp;
    tNickNameParam.strWxUserID = strWxUserID;
    tNickNameParam.strWxGroupID = strWxGroupID;

    CNickNameTask*  pNickNameTask = new CNickNameTask;
    pNickNameTask->SetTaskParam(tNickNameParam);

    auto callback = [](const tagNickNameParam & tParam) {
        if (!tParam.strNickName.empty()) {
            CString  szResp = stringToCString(tParam.tHttpResp.respText, CP_UTF8);  //_T("@") + CString(tParam.strNickName.c_str()) + _T(" ")
            LogInfo("execute nickname task callback : respType:{},wxGroupID:{}, wxUserID:{}, wxNickName:{} ,respContent:{}", tParam.tHttpResp.respType,
                    tParam.strWxGroupID, tParam.strWxUserID, tParam.strNickName, wstring2string(szResp.GetString()));
            CMsgManager::GetInstance()->SendAtMsg(tParam.tHttpResp.respType, CString(tParam.strWxGroupID.c_str()), CString(tParam.strWxUserID.c_str()), CString(tParam.strNickName.c_str()), szResp);


        } else {
            LogError("get nickname  error,userID:{}", tParam.strWxUserID);
        }
    };

    pNickNameTask->SetCallBack(callback);

    {
        std::lock_guard<std::mutex> lk(m_mapMutex);
        m_mapNickNameTask[strWxUserID].push_back(pNickNameTask);
    }


    _GetGroupMemberInfo(CString(strWxGroupID.c_str()), CString(strWxUserID.c_str()));
    return;
}


void  CNickNameTaskMgr::ActivateTask(const std::string& strGroupID, const std::string& strUserID) {
    if (m_mapNickNameTask.find(strUserID) == m_mapNickNameTask.end()) {
        LogError("activiate task  error");
        return;
    }

    if (g_WxData.mapGroupMemberInfo[strGroupID][strUserID].empty()) {
        LogError("not find user nickname: userGroupID:{} ,userWxID:{}", strGroupID, strUserID);
        return;
    }

    std::string userNickName = g_WxData.mapGroupMemberInfo[strGroupID][strUserID];

    for (auto& iter : m_mapNickNameTask[strUserID]) {
        iter->SetNickNameParam(userNickName);
        m_NotifyQueueService.addTask(iter);
    }

    {
        std::lock_guard<std::mutex> lk(m_mapMutex);
        m_mapNickNameTask[strUserID].clear();
    }

}

void  CNickNameTaskMgr::_GetGroupMemberInfo(CString wxGroupID, CString wxUserID) {

    //拷贝群成员微信ID
    RoomMemberInfo* pRoomMemberInfo = new RoomMemberInfo;
    wcscpy_s(pRoomMemberInfo->chatroomid, wcslen(wxGroupID.GetString()) + 1, wxGroupID.GetString());
    wcscpy_s(pRoomMemberInfo->wxid, wcslen(wxUserID.GetString()) + 1, wxUserID.GetString());

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return;
    }

    COPYDATASTRUCT show_members;
    show_members.dwData = WM_ShowChatRoomMember;
    show_members.cbData = sizeof(RoomMemberInfo);
    show_members.lpData = pRoomMemberInfo;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&show_members);

    return;

}