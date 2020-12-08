#include "stdafx.h"
#include <stddef.h>
#include "WndMsgLoop.h"
#include "InitWeChat.h"
#include "Login.h"
#include "MainWindow.h"
#include "MainWindow.h"
#include "FriendList.h"
#include "Function.h"
#include "ChatRoomOperate.h"
#include "CAutoFunction.h"
#include "base/ZLogger.h"
#include "base/util.h"
#include "CSaveImage.h"

extern BOOL g_AutoChat;


//************************************************************
// 函数名称: RegisterWindow
// 函数说明: 初始化窗口
// 作    者: GuiShou
// 时    间: 2019/6/30
// 参    数: HMODULE hModule 句柄
// 返 回 值: void
//************************************************************
void InitWindow(HMODULE hModule) {
    //检查当前微信版本
    if (IsWxVersionValid()) {
        //获取WeChatWin的基址
        DWORD dwWeChatWinAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll");

        //检测微信是否登陆
        DWORD dwIsLogin = dwWeChatWinAddr + LoginSign_Offset + 0x194;

        if (*(DWORD*)dwIsLogin == 0) {	//等于0说明微信未登录
            //开线程持续检测微信登陆状态
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckIsLogin, 0, 0, NULL);

            //HOOK获取好友列表的call
            HookGetFriendList();

            //HOOK接收消息
            HookChatRecord();

            //HOOK防撤回
            AntiRevoke();

            //HOOK提取表情
            HookExtractExpression(WxGetExpressionsAddr);

            //hook image save
            HookSaveImages(WxGetPicAddr);

            //注册窗口
            RegisterWindow(hModule);

        } else {
            //如果微信已经登陆 发送消息给客户端
            //查找登陆窗口句柄
            HWND hLogin = FindWindow(NULL, L"Login");

            if (hLogin == NULL) {
                OutputDebugStringA("未查找到Login窗口");
                return;
            }

            COPYDATASTRUCT login_msg;
            login_msg.dwData = WM_AlreadyLogin;
            login_msg.lpData = NULL;
            login_msg.cbData = 0;
            //发送消息给控制端
            SendMessage(hLogin, WM_COPYDATA, (WPARAM)hLogin, (LPARAM)&login_msg);
        }

    } else {
        MessageBoxA(NULL, "当前微信版本不匹配，请下载WeChat2.6.8.52", "错误", MB_OK);
    }

}


//************************************************************
// 函数名称: RegisterWindow
// 函数说明: 注册窗口
// 作    者: GuiShou
// 时    间: 2019/6/30
// 参    数: HMODULE hModule 窗口句柄
// 返 回 值: void
//************************************************************

void RegisterWindow(HMODULE hModule) {
    //1  设计一个窗口类
    WNDCLASS wnd;
    wnd.style = CS_VREDRAW | CS_HREDRAW;//风格
    wnd.lpfnWndProc = WndProc;//窗口回调函数指针.
    wnd.cbClsExtra = NULL;
    wnd.cbWndExtra = NULL;
    wnd.hInstance = hModule;
    wnd.hIcon = NULL;
    wnd.hCursor = NULL;
    wnd.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wnd.lpszMenuName = NULL;
    wnd.lpszClassName = TEXT("WeChatHelper");
    //2  注册窗口类
    RegisterClass(&wnd);
    //3  创建窗口
    HWND hWnd = CreateWindow(
                    TEXT("WeChatHelper"),  //窗口类名
                    TEXT("WeChatHelper"),//窗口名
                    WS_OVERLAPPEDWINDOW,//窗口风格
                    10, 10, 500, 300, //窗口位置
                    NULL,             //父窗口句柄
                    NULL,             //菜单句柄
                    hModule,        //实例句柄
                    NULL              //传递WM_CREATE消息时的附加参数
                );
    //4  更新显示窗口
    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);
    //5  消息循环（消息泵）
    MSG  msg = {};

    //   5.1获取消息
    while (GetMessage(&msg, 0, 0, 0)) {
        //   5.2翻译消息
        TranslateMessage(&msg);
        //   5.3转发到消息回调函数
        DispatchMessage(&msg);
    }
}


#if 0
//wchar_t转string
std::string Wchar_tToString(wchar_t* wchar) {
    std::string szDst;
    wchar_t* wText = wchar;
    DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用
    char* psText; // psText为char*的临时数组，作为赋值给std::string的中间变量
    psText = new char[dwNum];
    WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用
    szDst = psText;// std::string赋值
    delete[]psText;// psText的清除
    return szDst;
}
#endif

//************************************************************
// 函数名称: WndProc
// 函数说明: 回调函数 用于和控制端通信
// 作    者: GuiShou
// 时    间: 2019/6/30
// 参    数: HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam
// 返 回 值: LRESULT
//************************************************************
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    if (Message == WM_COPYDATA) {
        COPYDATASTRUCT* pCopyData = (COPYDATASTRUCT*)lParam;

        if (pCopyData == nullptr) {
            LogError("receiveMsg[WM_COPYDATA] error");
            return  DefWindowProc(hWnd, Message, wParam, lParam);
        }

        LogInfo("receiveMsg[WM_COPYDATA] id:{}  begin", pCopyData->dwData, pCopyData->cbData);

        switch (pCopyData->dwData) {
        //显示二维码
        case WM_ShowQrPicture: {
            LogInfo("receiveMsg[WM_ShowQrPicture]!");
            GotoQrCode();
            HookQrCode(QrCodeOffset);
        }
        break;

        //退出微信
        case WM_Logout: {
            LogInfo("receiveMsg[WM_Logout]!");
            LogoutWeChat();
        }
        break;

        //发送文本消息
        case WM_SendTextMessage: {

            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendTextMessage]: wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendTextMessage(textmessage->wxid, content);

        }
        break;

        //发送文件消息
        case WM_SendFileMessage: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendFileMessage], wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendFileMessage(textmessage->wxid, content);
        }
        break;

        //获取个人信息
        case WM_GetInformation: {
            LogInfo("receiveMsg[WM_GetInformation]");
            GetInformation();
        }
        break;

        //发送图片消息
        case WM_SendImageMessage: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendImageMessage],wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendImageMessage(textmessage->wxid, content);
        }
        break;

        //发送群公告
        case WM_SetRoomAnnouncement: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SetRoomAnnouncement],wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), wstring2string(content) ) ;
            SetWxRoomAnnouncement(textmessage->wxid, content);
        }
        break;

        //删除好友
        case WM_DeleteUser: {

            std::string strWxID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_DeleteUser],wxid:{}", strWxID);
            DeleteUser((wchar_t*)pCopyData->lpData);
        }
        break;

        //退出群聊
        case WM_QuitChatRoom: {

            std::string strWxID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_QuitChatRoom],wxid:{}", strWxID);
            QuitChatRoom((wchar_t*)pCopyData->lpData);
        }
        break;

        //添加群成员
        case WM_AddGroupMember: {
            struct AddGroupMem {
                wchar_t chatroomid[50];
                wchar_t wxid[50];
            };

            AddGroupMem* addgroupmember = (AddGroupMem*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_AddGroupMember],chatroomid:{},groupmemberid:{}", Wchar_tToString(addgroupmember->chatroomid), Wchar_tToString(addgroupmember->wxid));
            AddGroupMember(addgroupmember->chatroomid, addgroupmember->wxid);
        }
        break;

        //发送名片
        case WM_SendXmlCard: {
            struct XmlCardMessage {
                wchar_t RecverWxid[50];		//接收者的微信ID
                wchar_t SendWxid[50];		//需要发送的微信ID
                wchar_t NickName[50];		//昵称
            };

            XmlCardMessage* pCardMessage = (XmlCardMessage*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_SendXmlCard],receiveWxId:{},sendWxId:{},nickName:{}", Wchar_tToString(pCardMessage->RecverWxid), Wchar_tToString(pCardMessage->SendWxid), Wchar_tToString(pCardMessage->NickName));
            SendXmlCard(pCardMessage->RecverWxid, pCardMessage->SendWxid, pCardMessage->NickName);
        }
        break;

        //获取群某个成员信息
        case WM_ShowChatRoomMember: {
            struct   RoomMemberInfo {
                wchar_t chatroomid[50];
                wchar_t wxid[50];
            };

            RoomMemberInfo* pMember = (RoomMemberInfo*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_ShowChatRoomMember],chatroomid:{},memberWxId:{}", Wchar_tToString(pMember->chatroomid), Wchar_tToString(pMember->wxid));
            ShowChatRoomMember(pMember->chatroomid, pMember->wxid);
        }
        break;

        //显示群成员
        case WM_ShowChatRoomMembers: {

            std::string strChatRoomID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_ShowChatRoomMembers],chatroomid:{}", strChatRoomID);
            ShowChatRoomUser((wchar_t*)pCopyData->lpData);
        }
        break;

        case  WM_GetChatRoomMembers: {

            std::string strChatRoomID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_GetChatRoomMembers],chatroomid:{}", strChatRoomID);
            ShowChatRoomUser((wchar_t*)pCopyData->lpData);
        }
        break;

        //添加好友
        case WM_AddUser: {
            struct AddUserStruct {
                wchar_t wxid[50];
                wchar_t content[50];
            };
            AddUserStruct* addUser = (AddUserStruct*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_AddUser],wxid:{},content:{}", Wchar_tToString(addUser->wxid), Wchar_tToString(addUser->content));
            AddWxUser(addUser->wxid, addUser->content);
        }
        break;

        //修改群名称
        case WM_SetRoomName: {
            struct SetRoomNameStruct {
                wchar_t roomwxid[50];
                wchar_t roomname[50];
            };
            SetRoomNameStruct* setroomname = (SetRoomNameStruct*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_SetRoomName],wxGroupId:{},roomname:{}", Wchar_tToString(setroomname->roomwxid), Wchar_tToString(setroomname->roomname));
            SetRoomName(setroomname->roomwxid, setroomname->roomname);
        }
        break;

        //自动聊天
        case WM_AutoChat: {
            g_AutoChat = TRUE;
        }
        break;

        //取消自动聊天
        case WM_CancleAutoChat: {
            g_AutoChat = FALSE;
        }
        break;

        //发送艾特消息
        case WM_SendAtMsg: {
            struct AtMsg {
                wchar_t chatroomid[50] = { 0 };
                wchar_t memberwxid[50] = { 0 };
                wchar_t membernickname[50] = { 0 };
            };

            AtMsg* pAtMsg = (AtMsg*)pCopyData->lpData;
            std::wstring msgcontent = (wchar_t*)pCopyData->lpData + 150;

            LogInfo("receiveMsg[WM_SendAtMsg],wxGroupId:{},wxMemberID:{},roomname:{},content:{}", Wchar_tToString(pAtMsg->chatroomid), Wchar_tToString(pAtMsg->memberwxid), Wchar_tToString(pAtMsg->membernickname), wstring2string(msgcontent) );
            SendRoomAtMsg(pAtMsg->chatroomid, pAtMsg->memberwxid, pAtMsg->membernickname, msgcontent);
        }
        break;

        //删除群成员
        case WM_DelRoomMember: {
            struct DelMemberStruct {
                wchar_t roomid[50];
                wchar_t memberwxid[50];
            };
            DelMemberStruct* msg = (DelMemberStruct*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_DelRoomMember],wxGroupId:{},roomname:{}", Wchar_tToString(msg->roomid), Wchar_tToString(msg->memberwxid));
            DelRoomMember(msg->roomid, msg->memberwxid);
        }
        break;

        //打开URL
        case WM_OpenUrl: {
            LogInfo("receiveMsg[WM_OpenUrl]");
            OpenUrl((wchar_t*)pCopyData->lpData);
        }
        break;

        //保存联系人
        case WM_SaveFriendList: {
            LogInfo("receiveMsg[WM_SaveFriendList]");
            SaveToTxtFie();
        }
        break;

        default:
            break;
        }

        LogInfo("receiveMsg[WM_COPYDATA] id:{} end", pCopyData->dwData);
    }

    return DefWindowProc(hWnd, Message, wParam, lParam);
}




