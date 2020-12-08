#pragma once
#include <afxstr.h>
#include <string>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>
#include "COnlineCat.h"
#include "common/public_define.h"
#include "GroupTask.h"

using namespace std;

struct tagQueryInfo {
    int   queryType;
    std::string  wxSenderId;
    std::string  wxGroupId;
    std::string  wxQuestion;
};

enum  EnumChatMode {
    chat_mode_normal = 0, //正常@模式
    chat_mode_race = 1,  //抢答模式
    chat_mode_mix = 2,   //混合模式
};


struct tagAtMsg {
    wchar_t chatroomid[50] = { 0 };
    wchar_t memberwxid[50] = { 0 };
    wchar_t membernickname[50] = { 0 };
    wchar_t msgcontent[100] = { 0 };
};


class CMsgHelper {
  public:
    CMsgHelper();
    ~CMsgHelper();

    static CMsgHelper* GetInstance();
    void  Init();

    void  HandleChatMsg(const Message& msg);
    BOOL  HandlePrivateChatMsg(const Message& msg);
    void  HandleGroupChatMsg(const Message& msg, const string& szNickName);

    void  GetGroupMemberInfo(CString wxGroupID, CString wxUserID);
    void  QueryInfo(const tagQueryInfo& queryInfo, const HttpCallBack_Func& httpCallBack);

  public:
    //static void  addTask(std::string userID, CNickNameTask nickNameTask);
    void  addTask(const std::string& strWxGroupID, const std::string& strWxID, const tagHttpResp& tHttpResp);
    void  handleTask(const std::string& userID, const std::string& nickName);

  private:
    static CMsgHelper* instance_;
    //std::unordered_map<std::string, std::vector<CNickNameTask*> > m_MapTask;
    std::mutex  m_taskMutex;
    CNotifyQueueService m_pushMsgQueue;
};


