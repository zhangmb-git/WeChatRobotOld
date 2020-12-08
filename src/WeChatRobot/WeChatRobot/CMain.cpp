// CMain.cpp: 实现文件
//

#include "stdafx.h"
#include "Main.h"
#include "CMain.h"
#include "afxdialogex.h"
#include "CChatRecords.h"
#include "CFriendList.h"
#include "CFunctions.h"
#include "common/public_define.h"
#include "base/task_mgr.h"
#include "base/util.h"
#include "common/public_define.h"
#include <atlconv.h>
#include "pangmao/MsgHelper.h"
#include "CInformation.h"
#include "CSysConfig.h"
#include "base/ZLogger.h"
#include "pangmao/GroupTask.h"
#include "pangmao/NickNameTask.h"
#include "api/MsgReceive.h"
#include "api/MsgManager.h"
#include <thread>

//好友信息
struct UserInfo {
    wchar_t UserId[80];
    wchar_t UserNumber[80];
    wchar_t UserRemark[80];
    wchar_t UserNickName[80];
};


IMPLEMENT_DYNAMIC(CMain, CDialogEx)

CMain::CMain(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MAIN, pParent) {

}

CMain::~CMain() {
}

void CMain::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TAB1, m_MyTable);
}


BEGIN_MESSAGE_MAP(CMain, CDialogEx)
    ON_COMMAND(ID_32776, &CMain::OnWxLogout)
    ON_WM_COPYDATA()
    ON_COMMAND(ID_32798, &CMain::OnSaveFriendList)

END_MESSAGE_MAP()


// CMain 消息处理程序


BOOL CMain::OnInitDialog() {
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化
    TCHAR Name[3][5] = { L"好友列表", L"聊天记录", L"功能大全" };

    for (int i = 0; i < 3; i++) {
        m_MyTable.InsertItem(i, Name[i]);
    }

    //给子窗口指针赋值

    m_MyTable.m_Dia[0] = new CFriendList();
    m_MyTable.m_Dia[1] = new CChatRecords();
    m_MyTable.m_Dia[2] = new CFunctions();

    //创建子窗口
    UINT DiaIds[3] = { IDD_FRIEND_LIST, IDD_CHAT_RECORDS, IDD_FUNCTIONS};

    for (int i = 0; i < 3; i++) {
        m_MyTable.m_Dia[i]->Create(DiaIds[i], &m_MyTable);
    }


    //控制两个窗口的大小
    CRect rc;
    m_MyTable.GetClientRect(rc);
    rc.DeflateRect(2, 23, 2, 2);

    for (int i = 0; i < 3; i++) {
        m_MyTable.m_Dia[i]->MoveWindow(rc);
    }

    //显示第一个子窗口
    m_MyTable.m_Dia[0]->ShowWindow(SW_SHOW);

    for (int i = 1; i < 3; i++) {
        m_MyTable.m_Dia[i]->ShowWindow(SW_HIDE);
    }

    //获取个人信息
    CMsgManager::GetInstance()->GetPrivateInfo();

    // 5 s print my nick name
    std::thread t([]() {
        while (true) {
            if (!g_WxData.tPrivateInfo.nickname.empty()) {
                LogInfo("GetPrivateInfo success,my nickName={}", g_WxData.tPrivateInfo.nickname);
                break;

            } else {
                std::this_thread::sleep_for(200ms);
            }
        }
    });
    t.detach();

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常: OCX 属性页应返回 FALSE
}



//************************************************************
// 函数名称: OnWxLogout
// 函数说明: 响应退出微信菜单
// 作    者: GuiShou
// 时    间: 2019/6/30
// 参    数: void
// 返 回 值: void
//***********************************************************
void CMain::OnWxLogout() {
    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");
    COPYDATASTRUCT logout;
    logout.dwData = WM_Logout;
    logout.cbData = 0;
    logout.lpData = NULL;
    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&logout);
}


CString stringToCString(const std::string& src, UINT codepage) {
    CString dst;

    if (src.empty()) {
        return  dst;
    }

    int length = ::MultiByteToWideChar(codepage, 0, src.data(), (int)src.size(), NULL, 0);
    WCHAR* pBuffer = dst.GetBufferSetLength(length);
    ::MultiByteToWideChar(codepage, 0, src.data(), (int)src.size(), pBuffer, length);

    return dst;
}

//
std::string cStringToString(const CString& src, UINT codepage /*= CP_UTF8*/) {
    std::string dst;

    if (src.IsEmpty()) {
        dst.clear();
        return "";
    }

    int length = ::WideCharToMultiByte(codepage, 0, src, src.GetLength(), NULL, 0, NULL, NULL);
    dst.resize(length);
    ::WideCharToMultiByte(codepage, 0, src, src.GetLength(), &dst[0], (int)dst.size(), NULL, NULL);

    return dst;
}


//************************************************************
// 函数名称: OnCopyData
// 函数说明: 响应CopyData消息
// 作    者: GuiShou
// 时    间: 2019/7/5
// 参    数: CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct
// 返 回 值: BOOL
//***********************************************************
BOOL CMain::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) {
    // LogInfo("onCopyData ,pCopyDataStruct->dwData={}", pCopyDataStruct->dwData);

    //显示好友列表
    if (pCopyDataStruct->dwData == WM_GetFriendList) {
        UserInfo* user = new UserInfo;
        user = (UserInfo*)pCopyDataStruct->lpData;

        std::string strUserID = wstring2string(user->UserId);
        std::string strUserNickName = wstring2string(user->UserNickName);

        g_WxData.mapFriendList.insert(std::make_pair(strUserID, strUserNickName));

        //将消息发送给子窗口
        m_MyTable.m_Dia[0]->SendMessage(WM_ShowFriendList, (WPARAM)user, NULL);

    } else if (pCopyDataStruct->dwData == WM_GetInformation) {
        //接收消息
        Information* info = new Information;
        info = (Information*)pCopyDataStruct->lpData;

        if (!g_WxData.bGetInfoOK) {
            g_WxData.tPrivateInfo.wxid = Wchar_tToString(info->wxid);
            g_WxData.tPrivateInfo.wxcount = Wchar_tToString(info->wxcount);
            g_WxData.tPrivateInfo.nickname = Wchar_tToString(info->nickname);
            g_WxData.tPrivateInfo.phone = Wchar_tToString(info->phone);
            g_WxData.tPrivateInfo.device = Wchar_tToString(info->device);
            g_WxData.tPrivateInfo.nation = Wchar_tToString(info->nation);
            g_WxData.tPrivateInfo.city = Wchar_tToString(info->city);
            g_WxData.tPrivateInfo.province = Wchar_tToString(info->province);
            g_WxData.tPrivateInfo.header = Wchar_tToString(info->header);
            g_WxData.tPrivateInfo.wxsex = Wchar_tToString(info->wxsex);
            g_WxData.bGetInfoOK = true;
        }
    }

    //显示聊天记录
    else if (pCopyDataStruct->dwData == WM_ShowChatRecord) {
        Message* msg = new Message;
        msg = (Message*)pCopyDataStruct->lpData;

        //将消息发送给子窗口
        //m_MyTable.m_Dia[1]->SendMessage(WM_ShowMessage, (WPARAM)msg, NULL);

        if (msg->msgType != msg_type_text && msg->msgType != msg_type_pic &&
                msg->msgType != msg_type_radio && msg->msgType != msg_type_video &&
                msg->msgType != msg_type_voice && msg->msgType != msg_type_sharelink_file &&
                msg->msgType != msg_type_location) {
            LogInfo("onCopyData ,only unsupport text,pic,video,voic,location,file msg push,cur:{}", msg->msgType);
            return  FALSE;   // 不处理图片等其他消息 msg_type_pic
        }

        CMsgReceive::GetInstance()->OnHandleMsg(*msg);
        return true;

    } else if (pCopyDataStruct->dwData == WM_ShowChatRoomMember) {

        ChatRoomMemberInfo* user = (ChatRoomMemberInfo*)pCopyDataStruct->lpData;

        std::string  strGroupID = wstring2string(user->GroupId);
        std::string strUserID = wstring2string(user->UserId);
        std::string strUserNickname = wstring2string(user->UserNickName);

        g_WxData.mapGroupMemberInfo[strGroupID][strUserID] = strUserNickname;
        CNickNameTaskMgr::getInstance()->ActivateTask(strGroupID, strUserID);


    } else if (pCopyDataStruct->dwData == WM_ShowChatRoomMembers) {

        ChatRoomMemberInfo* user = (ChatRoomMemberInfo*)pCopyDataStruct->lpData;

        std::string  strGroupID = wstring2string(user->GroupId);
        std::string strUserID = wstring2string(user->UserId);
        std::string strUserNickname = wstring2string(user->UserNickName);
        g_WxData.mapGroupMemberInfo[strGroupID][strUserID] = strUserNickname;

        //m_MsgHelper.handleTask(strUserID, strUserNickname);

    } else if (pCopyDataStruct->dwData == WM_ShowChatRoomMembersDone) {

        std::string strChatRoomID = Wchar_tToString((wchar_t*)pCopyDataStruct->lpData);
        g_WxData.mapGroupMemberStatus[strChatRoomID] = true;

        CGroupTaskMgr::getInstance()->ActivateTask(strChatRoomID);
    }

    return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}


//************************************************************
// 函数名称: OnSaveFriendList
// 函数说明: 响应保存联系人菜单
// 作    者: GuiShou
// 时    间: 2019/9/17
// 参    数: void
// 返 回 值: void
//***********************************************************
void CMain::OnSaveFriendList() {
    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");
    COPYDATASTRUCT SaveFriendList;
    SaveFriendList.dwData = WM_SaveFriendList;
    SaveFriendList.cbData = 0;
    SaveFriendList.lpData = NULL;
    //发送消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&SaveFriendList);
}







