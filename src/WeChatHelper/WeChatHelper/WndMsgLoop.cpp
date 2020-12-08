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
// ��������: RegisterWindow
// ����˵��: ��ʼ������
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: HMODULE hModule ���
// �� �� ֵ: void
//************************************************************
void InitWindow(HMODULE hModule) {
    //��鵱ǰ΢�Ű汾
    if (IsWxVersionValid()) {
        //��ȡWeChatWin�Ļ�ַ
        DWORD dwWeChatWinAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll");

        //���΢���Ƿ��½
        DWORD dwIsLogin = dwWeChatWinAddr + LoginSign_Offset + 0x194;

        if (*(DWORD*)dwIsLogin == 0) {	//����0˵��΢��δ��¼
            //���̳߳������΢�ŵ�½״̬
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckIsLogin, 0, 0, NULL);

            //HOOK��ȡ�����б��call
            HookGetFriendList();

            //HOOK������Ϣ
            HookChatRecord();

            //HOOK������
            AntiRevoke();

            //HOOK��ȡ����
            HookExtractExpression(WxGetExpressionsAddr);

            //hook image save
            HookSaveImages(WxGetPicAddr);

            //ע�ᴰ��
            RegisterWindow(hModule);

        } else {
            //���΢���Ѿ���½ ������Ϣ���ͻ���
            //���ҵ�½���ھ��
            HWND hLogin = FindWindow(NULL, L"Login");

            if (hLogin == NULL) {
                OutputDebugStringA("δ���ҵ�Login����");
                return;
            }

            COPYDATASTRUCT login_msg;
            login_msg.dwData = WM_AlreadyLogin;
            login_msg.lpData = NULL;
            login_msg.cbData = 0;
            //������Ϣ�����ƶ�
            SendMessage(hLogin, WM_COPYDATA, (WPARAM)hLogin, (LPARAM)&login_msg);
        }

    } else {
        MessageBoxA(NULL, "��ǰ΢�Ű汾��ƥ�䣬������WeChat2.6.8.52", "����", MB_OK);
    }

}


//************************************************************
// ��������: RegisterWindow
// ����˵��: ע�ᴰ��
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: HMODULE hModule ���ھ��
// �� �� ֵ: void
//************************************************************

void RegisterWindow(HMODULE hModule) {
    //1  ���һ��������
    WNDCLASS wnd;
    wnd.style = CS_VREDRAW | CS_HREDRAW;//���
    wnd.lpfnWndProc = WndProc;//���ڻص�����ָ��.
    wnd.cbClsExtra = NULL;
    wnd.cbWndExtra = NULL;
    wnd.hInstance = hModule;
    wnd.hIcon = NULL;
    wnd.hCursor = NULL;
    wnd.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wnd.lpszMenuName = NULL;
    wnd.lpszClassName = TEXT("WeChatHelper");
    //2  ע�ᴰ����
    RegisterClass(&wnd);
    //3  ��������
    HWND hWnd = CreateWindow(
                    TEXT("WeChatHelper"),  //��������
                    TEXT("WeChatHelper"),//������
                    WS_OVERLAPPEDWINDOW,//���ڷ��
                    10, 10, 500, 300, //����λ��
                    NULL,             //�����ھ��
                    NULL,             //�˵����
                    hModule,        //ʵ�����
                    NULL              //����WM_CREATE��Ϣʱ�ĸ��Ӳ���
                );
    //4  ������ʾ����
    ShowWindow(hWnd, SW_HIDE);
    UpdateWindow(hWnd);
    //5  ��Ϣѭ������Ϣ�ã�
    MSG  msg = {};

    //   5.1��ȡ��Ϣ
    while (GetMessage(&msg, 0, 0, 0)) {
        //   5.2������Ϣ
        TranslateMessage(&msg);
        //   5.3ת������Ϣ�ص�����
        DispatchMessage(&msg);
    }
}


#if 0
//wchar_tתstring
std::string Wchar_tToString(wchar_t* wchar) {
    std::string szDst;
    wchar_t* wText = wchar;
    DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte������
    char* psText; // psTextΪchar*����ʱ���飬��Ϊ��ֵ��std::string���м����
    psText = new char[dwNum];
    WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte���ٴ�����
    szDst = psText;// std::string��ֵ
    delete[]psText;// psText�����
    return szDst;
}
#endif

//************************************************************
// ��������: WndProc
// ����˵��: �ص����� ���ںͿ��ƶ�ͨ��
// ��    ��: GuiShou
// ʱ    ��: 2019/6/30
// ��    ��: HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam
// �� �� ֵ: LRESULT
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
        //��ʾ��ά��
        case WM_ShowQrPicture: {
            LogInfo("receiveMsg[WM_ShowQrPicture]!");
            GotoQrCode();
            HookQrCode(QrCodeOffset);
        }
        break;

        //�˳�΢��
        case WM_Logout: {
            LogInfo("receiveMsg[WM_Logout]!");
            LogoutWeChat();
        }
        break;

        //�����ı���Ϣ
        case WM_SendTextMessage: {

            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendTextMessage]: wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendTextMessage(textmessage->wxid, content);

        }
        break;

        //�����ļ���Ϣ
        case WM_SendFileMessage: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendFileMessage], wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendFileMessage(textmessage->wxid, content);
        }
        break;

        //��ȡ������Ϣ
        case WM_GetInformation: {
            LogInfo("receiveMsg[WM_GetInformation]");
            GetInformation();
        }
        break;

        //����ͼƬ��Ϣ
        case WM_SendImageMessage: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SendImageMessage],wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), Wchar_tToString(content));
            SendImageMessage(textmessage->wxid, content);
        }
        break;

        //����Ⱥ����
        case WM_SetRoomAnnouncement: {
            MessageStruct* textmessage = (MessageStruct*)pCopyData->lpData;
            wchar_t* content = (wchar_t*)pCopyData->lpData + 40;
            LogInfo("receiveMsg[WM_SetRoomAnnouncement],wxid:{} ,content:{}", Wchar_tToString(textmessage->wxid), wstring2string(content) ) ;
            SetWxRoomAnnouncement(textmessage->wxid, content);
        }
        break;

        //ɾ������
        case WM_DeleteUser: {

            std::string strWxID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_DeleteUser],wxid:{}", strWxID);
            DeleteUser((wchar_t*)pCopyData->lpData);
        }
        break;

        //�˳�Ⱥ��
        case WM_QuitChatRoom: {

            std::string strWxID = Wchar_tToString((wchar_t*)pCopyData->lpData);
            LogInfo("receiveMsg[WM_QuitChatRoom],wxid:{}", strWxID);
            QuitChatRoom((wchar_t*)pCopyData->lpData);
        }
        break;

        //���Ⱥ��Ա
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

        //������Ƭ
        case WM_SendXmlCard: {
            struct XmlCardMessage {
                wchar_t RecverWxid[50];		//�����ߵ�΢��ID
                wchar_t SendWxid[50];		//��Ҫ���͵�΢��ID
                wchar_t NickName[50];		//�ǳ�
            };

            XmlCardMessage* pCardMessage = (XmlCardMessage*)pCopyData->lpData;
            LogInfo("receiveMsg[WM_SendXmlCard],receiveWxId:{},sendWxId:{},nickName:{}", Wchar_tToString(pCardMessage->RecverWxid), Wchar_tToString(pCardMessage->SendWxid), Wchar_tToString(pCardMessage->NickName));
            SendXmlCard(pCardMessage->RecverWxid, pCardMessage->SendWxid, pCardMessage->NickName);
        }
        break;

        //��ȡȺĳ����Ա��Ϣ
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

        //��ʾȺ��Ա
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

        //��Ӻ���
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

        //�޸�Ⱥ����
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

        //�Զ�����
        case WM_AutoChat: {
            g_AutoChat = TRUE;
        }
        break;

        //ȡ���Զ�����
        case WM_CancleAutoChat: {
            g_AutoChat = FALSE;
        }
        break;

        //���Ͱ�����Ϣ
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

        //ɾ��Ⱥ��Ա
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

        //��URL
        case WM_OpenUrl: {
            LogInfo("receiveMsg[WM_OpenUrl]");
            OpenUrl((wchar_t*)pCopyData->lpData);
        }
        break;

        //������ϵ��
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




