#include "stdafx.h"
#include "MsgManager.h"

#include "common/public_define.h"
#include "base/ZLogger.h"
#include "base/util.h"
#include "CSysConfig.h"
#include "base/file_util.h"
#include "common/HttpClient.h"

CMsgManager* CMsgManager::instance_ = new CMsgManager();

CMsgManager* CMsgManager::GetInstance() {
    return instance_;
}

bool CMsgManager::SendMsg(int nMsgType, const CString& wxid, const CString& szContent) {
    LogDebug("msgType:{},wxid:{},contentLen:{}", nMsgType, cStringToString(wxid), szContent.GetLength());


    //������ݵ��ṹ��
    int len = wcslen(szContent) + 40;
    wchar_t*    message = new wchar_t[len + 1];
    ::ZeroMemory(message, (len + 1) * sizeof(wchar_t));

    wcscpy_s(message, wcslen(wxid) + 1, wxid);
    wcscpy_s(message + 40, wcslen(szContent) + 1, szContent);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return false;
    }

    COPYDATASTRUCT MessageData;

    if (nMsgType == msg_type_text) { //�ı�
        MessageData.dwData = WM_SendTextMessage;

    } else if (nMsgType == msg_type_sharelink_file) { //�ļ�
        //����ļ��Ƿ����
        if (GetFileAttributes(szContent) == INVALID_FILE_ATTRIBUTES) {
            //MessageBox(L"�ļ������� ������");
            return false;
        }

        //�����ļ���Ϣ
        MessageData.dwData = WM_SendFileMessage;

    } else if (nMsgType == msg_type_pic) { //ͼƬ
        //���ͼƬ�Ƿ����
        if (GetFileAttributes(szContent) == INVALID_FILE_ATTRIBUTES) {
            //MessageBox(L"ͼƬ������ ������");
            return false;
        }

        //����ͼƬ��Ϣ
        MessageData.dwData = WM_SendImageMessage;

    } else if (nMsgType == msg_type_atallmsg) { //@��Ϣ
        MessageData.dwData = WM_SetRoomAnnouncement;
    }

    MessageData.cbData = (len + 1) * sizeof(wchar_t);
    MessageData.lpData = message;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;

}

bool CMsgManager::SendAtAllMsg(const CString& wxId, const CString& szContent) {
    LogDebug("wxid:{},contentLen:{}", cStringToString(wxId), szContent.GetLength());

    //������ݵ��ṹ��
    int len = wcslen(szContent) + 40;
    wchar_t*    message = new wchar_t[len + 1];
    ::ZeroMemory(message, (len + 1) * sizeof(wchar_t));

    wcscpy_s(message, wcslen(wxId) + 1, wxId);
    wcscpy_s(message + 40, wcslen(szContent) + 1, szContent);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find window wechathelper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.cbData = (len + 1) * sizeof(wchar_t);
    MessageData.lpData = message;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

//������Ƭ��Ϣ
bool CMsgManager::SendCard(const CString& wxReceiveID, const CString& wxCardID, const CString& wxNickName) {
    LogDebug("receiveId:{},cardId:{},nickName:{}", cStringToString(wxReceiveID), cStringToString(wxCardID), cStringToString(wxNickName));

    XmlCardMessage* cardMsg = new XmlCardMessage;
    wcscpy_s(cardMsg->RecverWxid, wcslen(wxReceiveID) + 1, wxReceiveID);
    wcscpy_s(cardMsg->SendWxid, wcslen(wxCardID) + 1, wxCardID);
    wcscpy_s(cardMsg->NickName, wcslen(wxNickName) + 1, wxNickName);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper window  nullptr");
        return  false;
    }

    COPYDATASTRUCT  MessageData;
    MessageData.dwData = WM_SendXmlCard;
    MessageData.cbData = sizeof(XmlCardMessage);
    MessageData.lpData = cardMsg;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);

    return  true;
}

//����@��Ϣ
bool CMsgManager::SendAtMsg(int nType, const CString wxGroupId, const CString& wxSenderId, const CString& nickName, const CString& szContent) {
    LogDebug("type:{},groupId:{},from:{},name:{},contentLen:{}", nType, cStringToString(wxGroupId), cStringToString(wxSenderId),
             cStringToString(nickName), szContent.GetLength());

    CString szAtContent;
    szAtContent.Format(L"@%s %s", nickName, szContent);
    //������ݵ��ṹ��
    int len = wcslen(szAtContent) + 150 ;
    wchar_t*    message = new wchar_t[len + 1];
    ::ZeroMemory(message, (len + 1) * sizeof(wchar_t));

    wcscpy_s(message, wcslen(wxGroupId) + 1, wxGroupId);
    wcscpy_s(message + 40, wcslen(szAtContent) + 1, szAtContent);
    //wcscpy_s(message + 50, wcslen(wxSenderId) + 1, wxSenderId);
    //wcscpy_s(message + 100, wcslen(nickName) + 1, nickName);
    //wcscpy_s(message + 150, wcslen(szAtContent) + 1, szAtContent);


    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return false;
    }

    COPYDATASTRUCT atmsgdata;
    atmsgdata.dwData = WM_SendTextMessage;    //WM_SendAtMsg;
    atmsgdata.cbData = (len + 1) * sizeof(wchar_t);
    atmsgdata.lpData = message;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&atmsgdata);
    return true;
}

bool CMsgManager::SendFriendDelMsg(const CString& wxFriendId) {
    LogInfo("frindId:{}", cStringToString(wxFriendId));

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT userOper;
    userOper.dwData = WM_DeleteUser;
    userOper.cbData = (wcslen(wxFriendId) + 1) * 2;
    userOper.lpData = (PVOID)wxFriendId.GetString();

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&userOper);
    return true;
}

//��Ӻ���
bool CMsgManager::SendFriendAddMsg(const CString& wxFriendId, const CString& szContent) {
    LogInfo("frindId:{},content:{}", cStringToString(wxFriendId), cStringToString(szContent));

    struct AddUserStruct {
        wchar_t wxid[50];
        wchar_t content[50];
    };

    AddUserStruct* addUser = new AddUserStruct;
    wcscpy_s(addUser->wxid, wcslen(wxFriendId) + 1, wxFriendId);
    wcscpy_s(addUser->content, wcslen(szContent) + 1, szContent);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT friendAdd;
    friendAdd.dwData = WM_AddUser;
    friendAdd.cbData = sizeof(AddUserStruct);
    friendAdd.lpData = addUser;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&friendAdd);
    return true;
}

//Ⱥ��Ա���� ��Ӻ�ɾ��Ⱥ��Ա
bool CMsgManager::SendGroupMemberOperMsg(int nType, const CString& wxGroupId, const CString& wxGroupMemberId) {
    LogInfo("type:{},groupId:{},memberId:{}", nType, cStringToString(wxGroupId), cStringToString(wxGroupMemberId));

    struct GroupMember {
        wchar_t chatroomid[50];
        wchar_t wxid[50];
    };

    //������ݵ��ṹ��
    GroupMember* message = new GroupMember;
    wcscpy_s(message->chatroomid, wcslen(wxGroupId) + 1, wxGroupId);
    wcscpy_s(message->wxid, wcslen(wxGroupMemberId) + 1, wxGroupMemberId);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;

    if (nType == E_Msg_AddGroupMember) {
        MessageData.dwData = WM_AddGroupMember;

    } else if (nType == E_Msg_DelRoomMember) {
        MessageData.dwData = WM_DelRoomMember;
    }

    MessageData.cbData = sizeof(GroupMember);
    MessageData.lpData = message;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

//�޸�Ⱥ����
bool CMsgManager::SendFixGroupNameMsg(const CString& wxGroupID, const CString& szRoomName) {
    LogInfo("groupId:{},roomName:{}", cStringToString(wxGroupID), cStringToString(szRoomName));

    struct SetRoomNameStruct {
        wchar_t roomwxid[50];
        wchar_t roomname[50];
    };

    SetRoomNameStruct* groupName = new SetRoomNameStruct;
    wcscpy_s(groupName->roomwxid, wcslen(wxGroupID) + 1, wxGroupID);
    wcscpy_s(groupName->roomname, wcslen(szRoomName) + 1, szRoomName);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_SetRoomAnnouncement;
    MessageData.cbData = sizeof(SetRoomNameStruct);
    MessageData.lpData = groupName;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

bool CMsgManager::SendGroupAnnounceMsg(const CString& wxid, const CString& content) {
    LogInfo("wxid:{},content:{}", cStringToString(wxid), cStringToString(content));

    MessageStruct* message = new MessageStruct;
    wcscpy_s(message->wxid, wcslen(wxid) + 1, wxid);
    wcscpy_s(message->content, wcslen(content) + 1, content);

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_SetRoomAnnouncement;
    MessageData.cbData = sizeof(MessageStruct);
    MessageData.lpData = message;

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

bool CMsgManager::SendGetGroupMemberListMsg(const CString& wxGroupID) {
    LogInfo("groupId:{}", cStringToString(wxGroupID));

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_GetChatRoomMembers;
    MessageData.cbData = (wcslen(wxGroupID) + 1) * 2;
    MessageData.lpData = (PVOID)wxGroupID.GetString();

    //������Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

void CMsgManager::GetPrivateInfo() {
    LogInfo("GetPrivateInfo");

    //���Ҵ���
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return;
    }

    COPYDATASTRUCT GetInformation;
    //��װ����
    GetInformation.dwData = WM_GetInformation;
    GetInformation.cbData = 0;
    GetInformation.lpData = NULL;
    //���ͻ�ȡ������Ϣ��Ϣ
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&GetInformation);
}

std::string CMsgManager::DownloadImageFromUrl(const std::string& url) {
    LogInfo("download image url:{}", url);

    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("load sysconfig error,null ptr");
        return  "";
    }

    CString  szUrl(url.c_str());
    CString  szLocalPath = getAppPath() + CString(pConfig->imageDir.c_str()) + _T("\\");
    PathUtil::CreateFolder_(wstring2string(szLocalPath.GetString()));

    int nPos = szUrl.ReverseFind('/');
    CString  szFileName = szUrl.Right(szUrl.GetLength() - nPos - 1);
    std::wstring wstrLocalFile = szLocalPath + szFileName;

    if (!::PathUtil::exist(wstring2string(wstrLocalFile))) {
        CHttpClient client;

        if (!client.DownloadByteFile(url, wstring2string(wstrLocalFile))) {
            LogError("download url error:{}", url);
            return "";

        } else {
            LogDebug("download 100% url:{},saveImage:{}", url, wstring2string(wstrLocalFile));
        }
    }

    return  wstring2string(wstrLocalFile);
}

std::string CMsgManager::DownloadFileFromUrl(int nMsgType, const std::string& url) {
    LogInfo("download file url:{}", url);

    tagConfig* pConfig = module::getSysConfigModule()->getSysConfig();

    if (pConfig == nullptr) {
        LogError("load sysconfig error");
        return  "";
    }

    CString  szUrl(url.c_str());
    CString  szLocalPath;
    szLocalPath = getAppPath() + CString(pConfig->fileDir.c_str()) + _T("\\");
    PathUtil::CreateFolder_(wstring2string(szLocalPath.GetString()));

    CString  szExtname = ::PathFindExtension(CString(url.c_str()));
    int nPos = szUrl.ReverseFind('/');
    CString  szFileName = szUrl.Right(szUrl.GetLength() - nPos - 1);
    std::wstring wstrLocalFile = szLocalPath + szFileName;

    if (!::PathUtil::exist(wstring2string(wstrLocalFile))) {
        CHttpClient client;

        if (!client.DownloadByteFile(url, wstring2string(wstrLocalFile))) {
            LogError("download url error:{}", url);
            return "";

        } else {
            LogDebug("download 100% url:{},saveFile:{}", url, wstring2string(wstrLocalFile));
        }
    }

    return  wstring2string(wstrLocalFile);
}