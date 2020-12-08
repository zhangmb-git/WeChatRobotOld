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


    //填充数据到结构体
    int len = wcslen(szContent) + 40;
    wchar_t*    message = new wchar_t[len + 1];
    ::ZeroMemory(message, (len + 1) * sizeof(wchar_t));

    wcscpy_s(message, wcslen(wxid) + 1, wxid);
    wcscpy_s(message + 40, wcslen(szContent) + 1, szContent);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return false;
    }

    COPYDATASTRUCT MessageData;

    if (nMsgType == msg_type_text) { //文本
        MessageData.dwData = WM_SendTextMessage;

    } else if (nMsgType == msg_type_sharelink_file) { //文件
        //检查文件是否存在
        if (GetFileAttributes(szContent) == INVALID_FILE_ATTRIBUTES) {
            //MessageBox(L"文件不存在 请重试");
            return false;
        }

        //发送文件消息
        MessageData.dwData = WM_SendFileMessage;

    } else if (nMsgType == msg_type_pic) { //图片
        //检查图片是否存在
        if (GetFileAttributes(szContent) == INVALID_FILE_ATTRIBUTES) {
            //MessageBox(L"图片不存在 请重试");
            return false;
        }

        //发送图片消息
        MessageData.dwData = WM_SendImageMessage;

    } else if (nMsgType == msg_type_atallmsg) { //@消息
        MessageData.dwData = WM_SetRoomAnnouncement;
    }

    MessageData.cbData = (len + 1) * sizeof(wchar_t);
    MessageData.lpData = message;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;

}

bool CMsgManager::SendAtAllMsg(const CString& wxId, const CString& szContent) {
    LogDebug("wxid:{},contentLen:{}", cStringToString(wxId), szContent.GetLength());

    //填充数据到结构体
    int len = wcslen(szContent) + 40;
    wchar_t*    message = new wchar_t[len + 1];
    ::ZeroMemory(message, (len + 1) * sizeof(wchar_t));

    wcscpy_s(message, wcslen(wxId) + 1, wxId);
    wcscpy_s(message + 40, wcslen(szContent) + 1, szContent);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find window wechathelper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.cbData = (len + 1) * sizeof(wchar_t);
    MessageData.lpData = message;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

//发送名片信息
bool CMsgManager::SendCard(const CString& wxReceiveID, const CString& wxCardID, const CString& wxNickName) {
    LogDebug("receiveId:{},cardId:{},nickName:{}", cStringToString(wxReceiveID), cStringToString(wxCardID), cStringToString(wxNickName));

    XmlCardMessage* cardMsg = new XmlCardMessage;
    wcscpy_s(cardMsg->RecverWxid, wcslen(wxReceiveID) + 1, wxReceiveID);
    wcscpy_s(cardMsg->SendWxid, wcslen(wxCardID) + 1, wxCardID);
    wcscpy_s(cardMsg->NickName, wcslen(wxNickName) + 1, wxNickName);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper window  nullptr");
        return  false;
    }

    COPYDATASTRUCT  MessageData;
    MessageData.dwData = WM_SendXmlCard;
    MessageData.cbData = sizeof(XmlCardMessage);
    MessageData.lpData = cardMsg;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);

    return  true;
}

//发送@消息
bool CMsgManager::SendAtMsg(int nType, const CString wxGroupId, const CString& wxSenderId, const CString& nickName, const CString& szContent) {
    LogDebug("type:{},groupId:{},from:{},name:{},contentLen:{}", nType, cStringToString(wxGroupId), cStringToString(wxSenderId),
             cStringToString(nickName), szContent.GetLength());

    CString szAtContent;
    szAtContent.Format(L"@%s %s", nickName, szContent);
    //填充数据到结构体
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

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&atmsgdata);
    return true;
}

bool CMsgManager::SendFriendDelMsg(const CString& wxFriendId) {
    LogInfo("frindId:{}", cStringToString(wxFriendId));

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT userOper;
    userOper.dwData = WM_DeleteUser;
    userOper.cbData = (wcslen(wxFriendId) + 1) * 2;
    userOper.lpData = (PVOID)wxFriendId.GetString();

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&userOper);
    return true;
}

//添加好友
bool CMsgManager::SendFriendAddMsg(const CString& wxFriendId, const CString& szContent) {
    LogInfo("frindId:{},content:{}", cStringToString(wxFriendId), cStringToString(szContent));

    struct AddUserStruct {
        wchar_t wxid[50];
        wchar_t content[50];
    };

    AddUserStruct* addUser = new AddUserStruct;
    wcscpy_s(addUser->wxid, wcslen(wxFriendId) + 1, wxFriendId);
    wcscpy_s(addUser->content, wcslen(szContent) + 1, szContent);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT friendAdd;
    friendAdd.dwData = WM_AddUser;
    friendAdd.cbData = sizeof(AddUserStruct);
    friendAdd.lpData = addUser;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&friendAdd);
    return true;
}

//群成员操作 添加和删除群成员
bool CMsgManager::SendGroupMemberOperMsg(int nType, const CString& wxGroupId, const CString& wxGroupMemberId) {
    LogInfo("type:{},groupId:{},memberId:{}", nType, cStringToString(wxGroupId), cStringToString(wxGroupMemberId));

    struct GroupMember {
        wchar_t chatroomid[50];
        wchar_t wxid[50];
    };

    //填充数据到结构体
    GroupMember* message = new GroupMember;
    wcscpy_s(message->chatroomid, wcslen(wxGroupId) + 1, wxGroupId);
    wcscpy_s(message->wxid, wcslen(wxGroupMemberId) + 1, wxGroupMemberId);

    //查找窗口
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

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

//修改群名称
bool CMsgManager::SendFixGroupNameMsg(const CString& wxGroupID, const CString& szRoomName) {
    LogInfo("groupId:{},roomName:{}", cStringToString(wxGroupID), cStringToString(szRoomName));

    struct SetRoomNameStruct {
        wchar_t roomwxid[50];
        wchar_t roomname[50];
    };

    SetRoomNameStruct* groupName = new SetRoomNameStruct;
    wcscpy_s(groupName->roomwxid, wcslen(wxGroupID) + 1, wxGroupID);
    wcscpy_s(groupName->roomname, wcslen(szRoomName) + 1, szRoomName);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_SetRoomAnnouncement;
    MessageData.cbData = sizeof(SetRoomNameStruct);
    MessageData.lpData = groupName;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

bool CMsgManager::SendGroupAnnounceMsg(const CString& wxid, const CString& content) {
    LogInfo("wxid:{},content:{}", cStringToString(wxid), cStringToString(content));

    MessageStruct* message = new MessageStruct;
    wcscpy_s(message->wxid, wcslen(wxid) + 1, wxid);
    wcscpy_s(message->content, wcslen(content) + 1, content);

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_SetRoomAnnouncement;
    MessageData.cbData = sizeof(MessageStruct);
    MessageData.lpData = message;

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

bool CMsgManager::SendGetGroupMemberListMsg(const CString& wxGroupID) {
    LogInfo("groupId:{}", cStringToString(wxGroupID));

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechat helper error");
        return false;
    }

    COPYDATASTRUCT MessageData;
    MessageData.dwData = WM_GetChatRoomMembers;
    MessageData.cbData = (wcslen(wxGroupID) + 1) * 2;
    MessageData.lpData = (PVOID)wxGroupID.GetString();

    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&MessageData);
    return true;
}

void CMsgManager::GetPrivateInfo() {
    LogInfo("GetPrivateInfo");

    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");

    if (pWnd == nullptr) {
        LogError("find wechathelper windows error");
        return;
    }

    COPYDATASTRUCT GetInformation;
    //组装数据
    GetInformation.dwData = WM_GetInformation;
    GetInformation.cbData = 0;
    GetInformation.lpData = NULL;
    //发送获取个人信息消息
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