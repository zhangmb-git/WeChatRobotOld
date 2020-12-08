#include "stdafx.h"
#include "COnlineCat.h"
#include "CSysConfig.h"
#include "common/HttpClient.h"
#include "base/util.h"
#include "json/json.h"
#include "common/public_define.h"
#include "MsgHelper.h"
#include "base/ZLogger.h"

std::string  COnlineCatTask::strWrongQuestion  = string_To_UTF8("问题没找到哦, 请输入正确问题");
std::string  COnlineCatTask::strConfigError    = string_To_UTF8("配置错误");
std::string  COnlineCatTask::strPostUrlError   = string_To_UTF8("请求网页错误");
std::string  COnlineCatTask::strParseDataError = string_To_UTF8("数据解析错误");
std::string  COnlineCatTask::strChartTip       = string_To_UTF8("图表");



bool COnlineCatTask::execute() {
    tagHttpResp  tHttpResp;
    _GetResponse(tHttpResp);

    if (m_Callback) {

        m_Callback(tHttpResp);
        //std::string  strRespText = Wchar_tToString(StringToWchar_t(tHttpResp.respText));
        //LogInfo("execute onlinecat task callback,respText:{},respType:{}", strRespText, tHttpResp.respType);
    }

    if (m_HttpCallback) {
        m_HttpCallback();
    }

    return  true;
}


void  COnlineCatTask::SetTaskParam(const tagTaskParam& taskParam) {
    m_taskParam = taskParam;
}

void   COnlineCatTask::SetCallBack(const CallBack_Func& func) {
    m_Callback = std::move(func);
}

void  COnlineCatTask::SetHttpCallBack(const HttpCallBack_Func& func) {
    m_HttpCallback = std::move(func);
}


std::string  COnlineCatTask::_ConstructBody() {
    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("config ptr null");
        return "";
    }

    USES_CONVERSION;
    Json::Value  root;
    root["msg"] = m_taskParam.msg;
    root["msgSenderId"]  = m_taskParam.msgSendId;
    root["msgSource"] = m_taskParam.msgSource;
    root["groupId"] = m_taskParam.groupId;
    root["incrementalID"] = m_taskParam.incrementalID;
    root["msgType"] = m_taskParam.msgType;
    root["sendTime"] = m_taskParam.sendTime;

    std::string  strBody = string_To_UTF8(root.toStyledString());
    return  strBody;
}

void  COnlineCatTask::_GetResponse(tagHttpResp& tHttpResp) {
    tHttpResp.respType = msg_type_text;
    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("config ptr null ");
        tHttpResp.respText = strConfigError;
        return  ;
    }

    if (pConfig->catUrl.empty()) {
        LogError("config catUrl empty ");
        tHttpResp.respText = strConfigError;
        return;
    }

    std::string  strUrl = string_To_UTF8(pConfig->catUrl);
    std::string  strData = _ConstructBody();


    std::string  strResp;
    CHttpClient  client;
    CURLcode code = client.PostJson(strUrl, strData, strResp);

    if (code != CURLE_OK) {
        LogError("post url err,url:{},data:{},respcode:{}", strUrl, strData, code);
        tHttpResp.respText = strPostUrlError;
        return  ;
    }

    Json::Reader  reader;
    Json::Value   root;

    if (!reader.parse(strResp.c_str(), root)) {
        LogError("parse url  resp  error,resp:{} ", strResp);
        tHttpResp.respText = strParseDataError;
        return  ;
    }

    LogInfo("post url:[{}],body:[{}], resp:[{}]", strUrl, UTF8_To_string(strData), UTF8_To_string(strResp));

    int  resCode = root.get("resCode", "").asInt();

    if (resCode == 0) {
        Json::Value  answerInfos;
        answerInfos = root.get("answerInfos", answerInfos);

        if (answerInfos.size() > 0) {
            tHttpResp.respText = answerInfos[0].get("content", "").asString();
            tHttpResp.respUrl = answerInfos[0].get("url", "").asString();
            tHttpResp.respType = answerInfos[0].get("msgType", "").asInt();

        } else {
            tHttpResp.respText = "answer info size 0";
        }

    } else {
        tHttpResp.respText = "您的问题不符合要求";
    }

    return ;
}



