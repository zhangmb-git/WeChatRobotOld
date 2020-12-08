/** @file MsgSender.h
  * @brief ��Ϣ���ͷ�װ
  * @author yingchun.xu
  * @date 2020/8/5
  */

#ifndef _MSGSENDER_DD6C1452_0D31_4EE8_BCAB_C9AAA2E61F68_
#define _MSGSENDER_DD6C1452_0D31_4EE8_BCAB_C9AAA2E61F68_

#include <string>

using namespace std;

class CMsgManager {
  public:
    static CMsgManager* GetInstance();

  public:
    // ��Ϣ���
    bool SendCard(const CString& wxReceiveID, const CString& wxCardID, const CString& wxCardNickName);
    bool SendMsg(int nType, const CString& wxSenderId, const CString& content);
    bool SendAtMsg(int nType, const CString wxGroupId, const CString& wxSenderId, const CString& nickName, const CString& content);
    bool SendAtAllMsg(const CString& wxId, const CString& content);

    // ����ҵ��
    bool SendFriendDelMsg(const CString& wxFriendId);
    bool SendFriendAddMsg(const CString& wxFriendId, const CString& szContent);
    bool SendGroupMemberOperMsg(int nType, const CString& wxGroupId, const CString& wxGroupMemberId);
    bool SendFixGroupNameMsg(const CString& wxGroupID, const CString& szRoomName);
    bool SendGroupAnnounceMsg(const CString& wxid, const CString& content);
    bool SendGetGroupMemberListMsg(const CString& wxGroupID);

    void GetPrivateInfo();

    std::string DownloadImageFromUrl(const std::string& url);
    std::string DownloadFileFromUrl(int nMsgType, const std::string& url);

  private:
    static CMsgManager* instance_;

  private:
    CMsgManager() = default;
    ~CMsgManager() = default;

    // remove copy
    CMsgManager(const CMsgManager&) = delete;
    CMsgManager(CMsgManager&&) = delete;
    CMsgManager operator=(CMsgManager&) = delete;
};

#endif//_MSGSENDER_DD6C1452_0D31_4EE8_BCAB_C9AAA2E61F68_