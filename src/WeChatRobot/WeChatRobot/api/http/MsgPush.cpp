#include "stdafx.h"

#include <atomic>
#include "MsgPush.h"

#include "common/public_define.h"
#include "api/MsgReceive.h"
#include "CSysConfig.h"
#include "base/ZLogger.h"
#include "base/util.h"

#include "common/HttpClient.h"
#include "json/json.h"

#include "base/common_define.hpp"
#include "base/thread_pool.h"

#define MAX_PUSH_THREAD (4)

struct  PushMsg {
    int    msgType;         //消息类型
    int    sessionType;     //1.私聊 2：群聊.
    int    sendTime;        //发送事件戳.

    std::string  strWxID;      //云智能猫微信ID
    std::string  msgContent;   //内容
    std::string  fromNickName; //发送人昵称
    std::string  fromID;       //发送人ID
    std::string  toID;         //接收人ID

};

void onReceiveMsg(const Message& msg);
void onExecutePushTask(void* msg);
bool isCatKeyOpen();
string getPushUrl();

CThreadPool g_pool;
std::atomic<uint32_t> g_task_id(0);

void StartHttpMsgPush() {
    LogInfo("start http_msg_push, thread num={},url={}", MAX_PUSH_THREAD, getPushUrl());

    CMsgReceive::GetInstance()->Register(onReceiveMsg, "StartMsgPush");
    // 4 个线程够了
    g_pool.init(MAX_PUSH_THREAD);
    g_pool.start();
}

void onReceiveMsg(const Message& msg) {
    // 推送消息
    int nPushFlag = module::getSysConfigModule()->getSysConfig()->pushFlag;

    if (!nPushFlag) {
        return;
    }

    string pushUrl = getPushUrl();

    if (pushUrl.empty()) {
        LogWarn("config pushUrl empty,push error");
        return;
    }

    LogInfo("from:{},to:{},msgData:{},url:{}", wstring2string(msg.msgSender), wstring2string(msg.wxid), wstring2string(msg.content), pushUrl.c_str());

    CString szContent(msg.content);
    CString szSource(msg.source);
    CString szWxID(msg.wxid);
    CString  szNickName;

    if (!g_WxData.bGetInfoOK || module::getSysConfigModule()->IsCatKeyOpen()) {
        szNickName = stringToCString(module::getSysConfigModule()->getSysConfig()->catKey, CP_UTF8);

    } else {
        szNickName = g_WxData.tPrivateInfo.nickname.c_str();
    }

    szNickName = _T("@") + szNickName;

    if (szContent.Find(szNickName) != -1) {
        szContent.Replace(szNickName, _T(""));
    }

    szContent.TrimLeft();
    szContent.TrimRight();

    PushMsg* param = new PushMsg;
    param->strWxID = g_WxData.tPrivateInfo.wxid;
    param->msgContent = cStringToString(szContent.GetString());
    param->toID = wstring2string(msg.wxid);
    param->fromID = wstring2string(msg.msgSender);
    param->fromNickName = "";
    //param->incrementalID = 0;
    param->msgType = msg.msgType;
    param->sendTime = (int)time(nullptr);
    std::string  strSource = wstring2string(msg.source);

    if (szSource.CompareNoCase(_T("群消息")) == 0) {
        param->sessionType = 2;

    } else {
        param->sessionType = 1;
    }

    CTask* t = new CTask();
    t->SetTaskParam((void*)param);
    t->SetCallBack(onExecutePushTask);

    g_pool.addTask(++g_task_id, t);
}


string getPushUrl() {
    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("config ptr null ");
        return "";
    }

    if (pConfig->pushUrl.empty()) {
        LogWarn("push url is empty");
        return "";
    }

    return pConfig->pushUrl;
}

string convertSessionType(int sessio_type) {
    switch (sessio_type) {
    case 1:
        return "single";

    case 2:
        return "group";

    default:
        return "unkonwn";
    }
}

string convertMsgType(int msgType) {
    switch (msgType) {
    case msg_type_text:
        return "text";

    case msg_type_pic:
        return "image";

    case msg_type_radio:
        return "short_video";

    case msg_type_video:
        return "video";

    case msg_type_voice:
        return "voice";

    case msg_type_location:
        return "location";

    case msg_type_sharelink_file:
        return "share_link_file";

    case msg_type_expression:
        return "emotion";

    default:
        return "unknown";
    }
}

void onExecutePushTask(void* msg) {
    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("config ptr null ");
        return;
    }

    if (pConfig->pushUrl.empty()) {
        LogError("config pushUrl empty ");
        return;
    }

    std::string  strUrl = string_To_UTF8(pConfig->pushUrl);

    PushMsg* data = (PushMsg*)msg;

    if (data == nullptr) {
        LogError("push msg data nullptr");
        return ;
    }

    Json::Value  param;
    param["wxId"] = data->strWxID;
    param["msgContent"] = data->msgContent;
    param["from"] = data->fromID;
    param["fromNickName"] = "";
    param["sessionType"] = convertSessionType(data->sessionType);
    param["to"] = data->toID;
    //param["clientMsgId"] = "guid";
    param["msgType"] = convertMsgType(data->msgType);
    param["sendTime"] = data->sendTime;

    std::string  strData = param.toStyledString();

    std::string  strResp;
    CHttpClient  client;
    CURLcode code = client.PostJson(strUrl, strData, strResp);

    if (code != CURLE_OK) {
        LogError("post url err,url:{},data:{},respcode:{}", strUrl, strData, code);
        return;
    }

    Json::Reader  reader;
    Json::Value   root;

    if (!reader.parse(strResp.c_str(), root)) {
        LogError("parse url  resp  error,resp:{} ", strResp);
        return;
    }

    LogInfo("post url:[{}],body:[{}], resp:[{}]", strUrl, UTF8_To_string(strData), UTF8_To_string(strResp));

    if (root["errorCode"].isNull() || root["errorMsg"].isNull()) {
        LogError("miss errorCode or errorMsg field ");
        return;
    }

    int  resCode = root.get("errorCode", "").asInt();
    std::string resErrMsg = root.get("errorMsg", "").asString();

    if (resCode != 0) {
        LogError("post url req:{},resp:{} err,errMsg:{}", strData, strResp, UTF8_To_string(resErrMsg));
    }
}