#pragma once
#include <string>
#include "base/common_thread.hpp"
#include "common/public_define.h"

typedef  std::function<void(const tagHttpResp& tHttpResp)>  CallBack_Func;
typedef  std::function<void()>  HttpCallBack_Func;


class COnlineCatTask : public  tagTask {
  public:

    virtual bool execute() override;
    virtual void onSuccess() {};
    virtual void onFail() {};

    void  SetTaskParam(const tagTaskParam& askParam);
    void  SetCallBack(const CallBack_Func& func);
    void  SetHttpCallBack(const HttpCallBack_Func& func);

  private:

    std::string  _ConstructBody();
    void  _GetResponse(tagHttpResp& tHttpResp);

  private:
    tagTaskParam  m_taskParam;

    static	std::string  strWrongQuestion;
    static	std::string  strConfigError;
    static	std::string  strPostUrlError;
    static	std::string  strParseDataError;
    static	std::string  strChartTip;

  private:
    //CString  m_szWxID;
    //CString  m_szContent;
    CallBack_Func  m_Callback;
    HttpCallBack_Func  m_HttpCallback;

};


