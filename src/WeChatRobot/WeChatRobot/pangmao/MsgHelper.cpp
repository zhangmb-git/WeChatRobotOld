#include "stdafx.h"
#include "MsgHelper.h"
#include "common/public_define.h"
#include "base/task_mgr.h"
#include "base/util.h"
#include "COnlineCat.h"
#include "CSysConfig.h"
#include "base/file_util.h"
#include "common/HttpClient.h"
#include "base/ZLogger.h"
#include "MsgTrafficControl.h"
#include "api/MsgManager.h"
#include "api/MsgReceive.h"
#include "NickNameTask.h"

CMsgHelper* CMsgHelper::instance_ = new CMsgHelper();

CMsgHelper::CMsgHelper() {
    m_pushMsgQueue.start();
}

CMsgHelper::~CMsgHelper() {
    m_pushMsgQueue.stop();
}

CMsgHelper* CMsgHelper::GetInstance() {
    return instance_;
}

void  CMsgHelper::Init() {
    auto callback = std::bind(&CMsgHelper::HandleChatMsg, this, std::placeholders::_1);
    CMsgReceive::GetInstance()->Register(callback, "CMsgHelper");

    LogInfo("胖猫问答功能已启动");
}


void  CMsgHelper::HandleChatMsg(const Message& msg) {
    CString szContent(msg.content);
    CString szSource(msg.source);
    CString szWxID(msg.wxid);

    LogInfo("HandleChatMsg ,from:{},to:{},msg:{}", wstring2string(msg.source), wstring2string(msg.wxid), wstring2string(msg.content));

    string  szNickName;

    if (!g_WxData.bGetInfoOK || module::getSysConfigModule()->IsCatKeyOpen()) {
        szNickName = module::getSysConfigModule()->getSysConfig()->catKey;

    } else {
        szNickName = g_WxData.tPrivateInfo.nickname.c_str();
    }

#if 0
    int nChatMode = module::getSysConfigModule()->getSysConfig()->chatMode;

    if (nChatMode == chat_mode_race) { //抢答模式
        if (szContent.Find(_T("@所有人")) != -1)  return FALSE;

        //非@胖猫不用回复
        if (szContent.Find(_T("@")) != -1 && szContent.Find(_T("@") + szNickName) == -1)  return FALSE;

        if (szSource.CompareNoCase(_T("群消息")) == 0 || szWxID.Find(_T("chatroom")) != -1) {
            return HandleGroupChatMsg(msg, szNickName);

        } else if (szSource.CompareNoCase(_T("好友消息")) == 0) {
            return HandlePrivateChatMsg(msg);
        }

    } else if (nChatMode == chat_mode_normal) {
        if (szSource.CompareNoCase(_T("群消息")) == 0 || szWxID.Find(_T("chatroom")) != -1) {
            if (szContent.Find(_T("@") + szNickName) != -1) {
                return HandleGroupChatMsg(msg, szNickName);
            }

        } else if (szSource.CompareNoCase(_T("好友消息")) == 0) {
            return HandlePrivateChatMsg(msg);
        }

    } else {
        if (szSource.CompareNoCase(_T("群消息")) == 0) {
            return HandleGroupChatMsg(msg, szNickName);

        } else if (szSource.CompareNoCase(_T("好友消息")) == 0) {
            return HandlePrivateChatMsg(msg);
        }
    }

#else

    if (szContent.Find(_T("@所有人")) != -1) {
        LogInfo("@ALL msg,drop it");
        return;
    }

    //非@胖猫不用回复
    if (szSource.CompareNoCase(_T("群消息")) == 0 || szWxID.Find(_T("chatroom")) != -1) {
        HandleGroupChatMsg(msg, szNickName);

    } else if (szSource.CompareNoCase(_T("好友消息")) == 0) {
        HandlePrivateChatMsg(msg);
    }

#endif
}

//处理群聊消息
void CMsgHelper::HandleGroupChatMsg(const Message& msg, const string& szNickName) {
    LogInfo("ReceiveGroupMsg: msgType:{},type:{},source:{},wxID:{},msgSenderID:{},content:{}", msg.msgType,
            wstring2string(msg.type), wstring2string(msg.source), wstring2string(msg.wxid),
            wstring2string(msg.msgSender), wstring2string(msg.content));

    if (!module::getSysConfigModule()->IsGroupChatOpen()) {
        LogWarn("group chat not open");
        return ;
    }

    CString wxGroupID(msg.wxid);
    CString wxSenderID(msg.msgSender);
    string szContent(wstring2string(msg.content));

    //处理文本
    int nChatMode = module::getSysConfigModule()->getSysConfig()->chatMode;

    if (nChatMode == 0 && szContent.find("@" + szNickName) == -1) {
        LogWarn("not at me,but @{}", szNickName);
        return ;
    }

    replace_all(szContent, "@" + szNickName, "");
    szContent = trim(szContent);

    // 群聊的时候，启用流控。单聊先暂时不启用。
    bool start_limit = false;
    std::string limit_answer = "";

    if (!CMsgTrafficControl::Grant(start_limit, limit_answer)) {
        if (start_limit) {
            CString msg(limit_answer.c_str()); //stringToCString(limit_answer, CP_UTF8);
            CMsgManager::GetInstance()->SendMsg(msg_type_text, wxGroupID, msg);
            LogWarn("start limit,from:{},msg:{},answer:{}", wxGroupID, szContent, limit_answer);

        } else {
            LogWarn("in limit status,from:{},msg:{}", wxGroupID, szContent);
        }

        return ;
    }

    if (msg.msgType == msg_type_text) {

        auto callback = [this, wxGroupID, wxSenderID](const tagHttpResp & resp) {
            LogDebug("respType:{}", resp.respType);

            CString  szResp;

            if (resp.respType == msg_type_pic) {
                szResp = CMsgManager::GetInstance()->DownloadImageFromUrl(resp.respUrl).c_str();

            } else if (resp.respType == msg_type_sharelink_file) {
                szResp = CMsgManager::GetInstance()->DownloadFileFromUrl(resp.respType, resp.respUrl).c_str();

            } else if (resp.respType == msg_type_text) {
                std::string  strGroupID = wstring2string(wxGroupID.GetString());
                std::string  strSenderID = wstring2string(wxSenderID.GetString());
                std::string  strNickName = g_WxData.mapFriendList[strSenderID];

                if (strNickName.empty() && strcmp(strSenderID.c_str(), "NULL") != 0) {
                    strNickName = g_WxData.mapGroupMemberInfo[strGroupID][strSenderID];
                }

                if (!strNickName.empty()) {
                    CString szNickName(strNickName.c_str());
                    szResp =  stringToCString(resp.respText, CP_UTF8); //_T("@") + szNickName + _T(" ") +
                    LogInfo("execute online task callback handle resp msg: groupID:{} respType:{},respContent:{}", wstring2string(wxGroupID.GetString()),
                            resp.respType, wstring2string(szResp.GetString()));

                    CMsgManager::GetInstance()->SendAtMsg(resp.respType, wxGroupID, wxSenderID, szNickName, szResp);

                    return;

                } else if (strSenderID != "NULL") {
                    LogWarn("get nickname, fromId:{}", strSenderID);
                    std::string strWxGroupID = wstring2string(wxGroupID.GetString());
                    CNickNameTaskMgr::getInstance()->addTask(strWxGroupID, strSenderID, resp);
                    return;

                } else { //strSenderID为NULL
                    szResp = stringToCString(resp.respText, CP_UTF8);
                }
            }

            if ((resp.respType == msg_type_pic || resp.respType == msg_type_sharelink_file) && szResp.IsEmpty()) {
                szResp = _T("下载文件错误:") + stringToCString(resp.respUrl, CP_UTF8);
            }

            LogInfo("execute group chat task callback handle resp msg: groupID:{} respType:{},respContent:{}", wstring2string(wxGroupID.GetString()),
                    resp.respType, wstring2string(szResp.GetString()));
            CMsgManager::GetInstance()->SendMsg(resp.respType, wxGroupID, szResp);
            return;

        };

        tagTaskParam  tTaskParam;
        tTaskParam.groupId = wstring2string(msg.wxid);
        tTaskParam.msgSendId = wstring2string(msg.msgSender);
        tTaskParam.incrementalID = 1;
        tTaskParam.msg = szContent;
        tTaskParam.msgSource = 2; //群聊
        tTaskParam.sendTime = (int)(time(nullptr));
        tTaskParam.msgType = msg.msgType;

        COnlineCatTask*  pOnlineCatTask = new COnlineCatTask;
        pOnlineCatTask->SetTaskParam(tTaskParam);
        pOnlineCatTask->SetCallBack(callback);
        CTaskMgr::getInstance()->AddTask(cStringToString(wxSenderID, CP_UTF8), pOnlineCatTask);
    }
}

//处理私聊消息
BOOL  CMsgHelper::HandlePrivateChatMsg(const Message& msg) {

    LogInfo("ReceivePrivateMsg: msgType:{},type:{},source:{},wxID:{},msgSenderID:{},content:{}", msg.msgType,
            wstring2string(msg.type), wstring2string(msg.source), wstring2string(msg.wxid),
            wstring2string(msg.msgSender), wstring2string(msg.content));

    if (!module::getSysConfigModule()->IsPrivateChatOpen()) {
        LogWarn("private chat not  open");
        return  FALSE;
    }

    // 群聊的时候，启用流控。单聊先暂时不启用。
    bool start_limit = false;
    std::string limit_answer = "";
    CString wxGroupID(msg.wxid);
    CString szContent(msg.content);

    if (!CMsgTrafficControl::Grant(start_limit, limit_answer)) {
        if (start_limit) {
            CString msg(limit_answer.c_str()); // stringToCString(limit_answer, CP_UTF8);
            CMsgManager::GetInstance()->SendMsg(msg_type_text, wxGroupID, msg);
            LogWarn("start limit,from:{},msg:{},answer:{}", wxGroupID, wstring2string(szContent.GetString()), limit_answer);

        } else {
            LogWarn("in limit status,from:{},msg:{}", wxGroupID, wstring2string(szContent.GetString()));
        }

        return FALSE;
    }

    CString wxID(msg.wxid);
    auto callback = [this, wxID](const tagHttpResp & resp) {

        CString  szResp;

        if (resp.respType == msg_type_pic) {
            szResp = CMsgManager::GetInstance()->DownloadImageFromUrl(resp.respUrl).c_str();

        } else if (resp.respType == msg_type_sharelink_file) {
            szResp = CMsgManager::GetInstance()->DownloadFileFromUrl(resp.respType, resp.respUrl).c_str();

        } else if (resp.respType == msg_type_text) {
            szResp = stringToCString(resp.respText, CP_UTF8);
        }

        if ((resp.respType == msg_type_pic || resp.respType == msg_type_sharelink_file) && szResp.IsEmpty()) {
            szResp = _T("下载文件错误:") + stringToCString(resp.respUrl, CP_UTF8);
        }

        LogInfo("execute private chat task callback handle resp msg: respType:{},wxID:{} ,respContent:{}", resp.respType,
                wstring2string(wxID.GetString()), wstring2string(szResp.GetString()));
        CMsgManager::GetInstance()->SendMsg(resp.respType, wxID, szResp);
    };

    tagTaskParam  tTaskParam;
    tTaskParam.groupId = wstring2string(msg.wxid);
    tTaskParam.msgSendId = wstring2string(msg.msgSender);
    tTaskParam.incrementalID = 1;
    tTaskParam.msg = wstring2string(szContent.GetString());
    tTaskParam.msgSource = 1; //单聊
    tTaskParam.sendTime = (int)(time(nullptr));
    tTaskParam.msgType = msg.msgType;

    COnlineCatTask*  pOnlineCatTask = new COnlineCatTask;
    pOnlineCatTask->SetTaskParam(tTaskParam);
    pOnlineCatTask->SetCallBack(callback);
    CTaskMgr::getInstance()->AddTask(cStringToString(wxID, CP_UTF8), pOnlineCatTask);
    return TRUE;
}

//添加获取昵称任务
//void  CMsgHelper::addTask(const std::string& strWxGroupID, const std::string& strWxUserID, const tagHttpResp& resp) {
//    if (g_WxData.mapFriendList.find(strWxGroupID) == g_WxData.mapFriendList.end())  return;
//
//    tagNickNameParam  tNickNameParam;
//    tNickNameParam.tHttpResp = resp;
//    tNickNameParam.strWxUserID = strWxUserID;
//    tNickNameParam.strWxGroupID = strWxGroupID;
//
//    CNickNameTask  nickNameTask;
//    nickNameTask.SetTaskParam(tNickNameParam);
//
//    auto callback = [](const tagNickNameParam & tParam) {
//        if (!tParam.strNickName.empty()) {
//            CString  szResp =  stringToCString(tParam.tHttpResp.respText, CP_UTF8);  //_T("@") + CString(tParam.strNickName.c_str()) + _T(" ")
//            LogInfo("execute nickname task callback : respType:{},wxGroupID:{}, wxUserID:{}, wxNickName:{} ,respContent:{}", tParam.tHttpResp.respType,
//                    tParam.strWxGroupID, tParam.strWxUserID, tParam.strNickName, wstring2string(szResp.GetString()) );
//            CMsgManager::GetInstance()->SendAtMsg(tParam.tHttpResp.respType, CString(tParam.strWxGroupID.c_str()), CString(tParam.strWxUserID.c_str()), CString(tParam.strNickName.c_str()), szResp);
//
//        } else {
//            LogError("get nickname  error,userID:{}", tParam.strWxUserID);
//        }
//    };
//
//    nickNameTask.SetCallBack(callback);
//    {
//        std::lock_guard<std::mutex> lck(m_taskMutex);
//        m_MapTask[strWxUserID].push_back(nickNameTask);
//
//    }
//
//    GetGroupMemberInfo(CString(strWxGroupID.c_str()), CString(strWxUserID.c_str()));
//    return;
//}
//
//void  CMsgHelper::handleTask(const std::string& userID, const std::string& nickName) {
//    LogInfo("nickname task size: {},handle userID:{} ,nickName:{}", m_MapTask.size(), userID, nickName);
//
//    if (nickName.empty() || strcmp(nickName.c_str(), "NULL") == 0) {
//        LogError("nickName error");
//        return;
//    }
//
//    auto iterUser = m_MapTask.find(userID);
//
//    if (iterUser != m_MapTask.end()) {
//        for (auto& iterTask : iterUser->second) {
//            iterTask.SetNickNameParam(nickName);
//            iterTask.execute();
//        }
//
//    }
//
//    {
//        std::lock_guard<std::mutex>  lck(m_taskMutex);
//        m_MapTask.erase(userID);
//    }
//
//    return;
//}

void CMsgHelper::QueryInfo(const tagQueryInfo& queryInfo, const HttpCallBack_Func& httpCallBack) {

    CString szWxContent(queryInfo.wxQuestion.c_str());
    CString szWxGroupID(queryInfo.wxGroupId.c_str());
    CString szWxSenderID(queryInfo.wxSenderId.c_str());

    auto callback = [ = ](const tagHttpResp & resp) {

        std::string  strNickName = g_WxData.mapFriendList[queryInfo.wxSenderId];
        std::string  strResp = resp.respText;

        CString  szResp = stringToCString(strResp, CP_UTF8);

        if (!strNickName.empty()) {
            USES_CONVERSION;
            szResp = _T("@") + CString(strNickName.c_str()) + _T(" ") + szResp;
        }

        CMsgManager::GetInstance()->SendMsg(1, szWxGroupID, szResp);

    };

    tagTaskParam  tTaskParam;
    tTaskParam.groupId = queryInfo.wxGroupId;
    tTaskParam.incrementalID = 1;
    tTaskParam.msg = queryInfo.wxQuestion;
    tTaskParam.msgSendId = queryInfo.wxSenderId;
    tTaskParam.msgSource = 1;
    tTaskParam.msgType = queryInfo.queryType;
    tTaskParam.sendTime = (int)(time(nullptr));

    COnlineCatTask*  pOnlineCatTask = new COnlineCatTask;
    pOnlineCatTask->SetTaskParam(tTaskParam);
    pOnlineCatTask->SetCallBack(callback);
    pOnlineCatTask->SetHttpCallBack(httpCallBack);
    CTaskMgr::getInstance()->AddTask(queryInfo.wxSenderId, pOnlineCatTask);
    return;
}

