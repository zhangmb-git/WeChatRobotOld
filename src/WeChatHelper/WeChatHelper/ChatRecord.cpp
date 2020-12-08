#include "stdafx.h"
#include <string>
#include <Shlwapi.h>
#include "ChatRecord.h"
#include "FriendList.h"
#include "CAutoFunction.h"
#include "base/ZLogger.h"
#include "base/util.h"
#include "base/CSysConfig.h"
#include "GlobalDef.h"
#include <stdio.h>
#pragma comment(lib,"Shlwapi.lib")
using namespace std;


BOOL g_AutoChat = FALSE;	//�Ƿ��Զ�����
BOOL isSendTuLing = FALSE;	//�Ƿ��Ѿ�������ͼ�������
BOOL isText = TRUE;			//�Ƿ���������Ϣ
wchar_t tempwxid[50] = { 0 };	//���΢��ID

DWORD r_esp = 0;
DWORD r_eax = 0;

CHAR originalCode[5] = { 0 };
//������ƫ��
DWORD dwParam = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessageParam;

//������ҪHOOK�ĵ�ַ
DWORD dwHookAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessage - 5;

//���ص�ַ
DWORD RetAddr = dwHookAddr + 5;

enum WxCommandType {
    kCommandTypeText = 0x01,
    kCommandTypeImage = 0x03,
    kCommandTypeVoice = 0x22,
    kCommandTypeCard = 0x2A,
    kCommandTypeShortVideo = 0x3E, // С��Ƶ
    kCommandTypeVideo = 0x2B, // ��Ƶ
    kCommandTypeEmotion = 0x2F, // ����
    kCommandTypeLocation = 0x30, // λ��
    kCommandTypeSharelinkFile = 0x31, // ����ʵʱλ�á��ļ���ת�ˡ�����

    kCommandTypeRecallMsg = 0x2712, // ������Ϣ

    kCommandTypeVoipMsg = 0x32,
    kCommandTypeVoipNotify = 0x34, //VOIPNOTIFY
    kCommandTypeVoipInvite = 0x35, // VOIPINVITE,

    kCommandTypeFriendAck = 0x25,
    kCommandTypeFriendPossible = 0x28,

    kCommandTypeWeChatInit = 0x33,

    kCommandTypeSysNotice = 0x270F, // ϵͳ֪ͨ
    kCommandTypeSysMsg = 0x2710, // �����ϵͳ��Ϣ
};

//************************************************************
// ��������: HookChatRecord
// ����˵��: HOOK�����¼
// ��    ��: GuiShou
// ʱ    ��: 2019/7/6
// ��    ��: void
// �� �� ֵ: void
//************************************************************
void HookChatRecord() {
    //��װ����
    BYTE bJmpCode[5] = { 0xE9 };
    *(DWORD*)&bJmpCode[1] = (DWORD)RecieveWxMesage - dwHookAddr - 5;

    //���浱ǰλ�õ�ָ��,��unhook��ʱ��ʹ�á�
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, originalCode, 5, 0);

    //����ָ�� B9 E8CF895C //mov ecx,0x5C89CFE8
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, bJmpCode, 5, 0);
}



//************************************************************
// ��������: RecieveMesage
// ����˵��: ������Ϣ
// ��    ��: GuiShou
// ʱ    ��: 2019/7/6
// ��    ��: void
// �� �� ֵ: void
//************************************************************
__declspec(naked) void RecieveWxMesage() {
    //�����ֳ�
    __asm {
        //���䱻���ǵĴ���
        //5B950573  |.  B9 E8CF895C           mov ecx,WeChatWi.5C89CFE8
        //mov ecx,0x5C89CFE8
        mov ecx, dwParam

        //��ȡesp�Ĵ������ݣ�����һ��������
        mov r_esp, esp
        mov r_eax, eax

        pushad
        pushfd
    }
    SendWxMessage();

    //�ָ��ֳ�
    __asm {
        popfd
        popad
        //���ر�HOOKָ�����һ��ָ��
        jmp RetAddr
    }
}

wstring GetMsgTypeStr(int msg_type, bool& is_other) {
    switch (msg_type) {
    case kCommandTypeText:
        return L"����";

    case kCommandTypeImage:
        return L"ͼƬ";

    case kCommandTypeVoice:
        return  L"����";

    case kCommandTypeFriendAck:
        return L"����ȷ��";

    case kCommandTypeFriendPossible:
        is_other = true;
        return L"POSSIBLEFRIEND_MSG";

    case kCommandTypeCard:
        return L"��Ƭ";

    case kCommandTypeVideo:
        return L"��Ƶ";

    case kCommandTypeEmotion:
        //ʯͷ������
        return L"����";

    case kCommandTypeLocation:
        return L"λ��";

    case kCommandTypeSharelinkFile:
        //����ʵʱλ��
        //�ļ�
        //ת��
        //����
        //�տ�
        //is_other = true;
        return L"����ʵʱλ�á��ļ���ת�ˡ�����";

    case kCommandTypeVoipMsg:
        is_other = true;
        return  L"VOIPMSG";

    case kCommandTypeWeChatInit:
        is_other = true;
        return L"΢�ų�ʼ��";

    case kCommandTypeVoipNotify:
        is_other = true;
        return  L"VOIPNOTIFY";

    case kCommandTypeVoipInvite:
        is_other = true;
        return L"VOIPINVITE";

    case kCommandTypeShortVideo:
        return L"С��Ƶ";

    case kCommandTypeSysNotice:
        is_other = true;
        return L"SYSNOTICE";

    case kCommandTypeSysMsg:
        //ϵͳ��Ϣ
        //���
        return L"�����ϵͳ��Ϣ";

    case kCommandTypeRecallMsg:
        return L"������Ϣ";

    default:
        is_other = true;
        return L"δ֪";
    }
}

//************************************************************
// ��������: SendMessage
// ����˵��: �����յ�����Ϣ���͸��ͻ���
// ��    ��: GuiShou
// ʱ    ��: 2019/7/6
// ��    ��: void
// �� �� ֵ: void
//************************************************************
void SendWxMessage() {
    Message* msg = new Message;
    ::memset(msg, 0, sizeof(msg));

    // ��Ϣ���λ��
    DWORD** msgAddress = (DWORD**)r_esp;
    // ��Ϣ����
    DWORD msgType = *((DWORD*)(**msgAddress + 0x30));
    msg->msgType = msgType;

    // ΢��ID/ȺID
    LPVOID pWxid = *((LPVOID*)(**msgAddress + 0x40));
    swprintf_s(msg->wxid, L"%s", (wchar_t*)pWxid);

    // ��Ϣ����
    wstring fullmessgaedata = GetMsgByAddress(**msgAddress + 0x68);	//��������Ϣ����
    //�ж���Ϣ��Դ��Ⱥ��Ϣ���Ǻ�����Ϣ
    wstring msgSource2 = L"<msgsource />\n";
    wstring msgSource = L"";
    msgSource.append(GetMsgByAddress(**msgAddress + 0x168));

    //LogInfo("receive msg,msgType:{},desc:{},content:{},", msgType, wstring2string(msg->type), wstring2string(fullmessgaedata));

    // ��Ϣ�����ı�����
    bool is_other_msg = false;
    wstring desc = GetMsgTypeStr(msg->msgType, is_other_msg);
    memcpy(msg->type, desc.c_str(), desc.length());

    //������Ϣ
    if (!msgSource.empty() && msgSource.length() <= msgSource2.length()) {
        memcpy(msg->source, L"������Ϣ", sizeof(L"������Ϣ"));
        memcpy(msg->msgSender, L"NULL", sizeof(L"NULL"));

    } else {
        // Ⱥ��Ϣ
        memcpy(msg->source, L"Ⱥ��Ϣ", sizeof(L"Ⱥ��Ϣ"));

        // ��ʾ��Ϣ������
        LPVOID pSender = *((LPVOID*)(**msgAddress + 0x114));
        swprintf_s(msg->msgSender, L"%s", (wchar_t*)pSender);
    }

    // ��ʾ��Ϣ����  �����޷���ʾ����Ϣ ��ֹ����
    if (StrStrW(msg->wxid, L"gh")) {
        //���΢��IDΪgh_3dfda90e39d6 ˵�����տ���Ϣ
        if ((StrCmpW(msg->wxid, L"gh_3dfda90e39d6") == 0)) {
            wstring desc = L"΢���տ��";
            wcscpy(msg->content, desc.c_str());
            msg->isMoney = TRUE;

        } else {
            //���΢��ID�д���gh ˵���ǹ��ں�
            wstring desc = L"���ںŷ�������,�����ֻ��ϲ鿴";
            wcscpy(msg->content, desc.c_str());
        }

    } else if (is_other_msg) {
        //ȡ����Ϣ����
        wchar_t tempcontent[0x10000] = { 0 };
        LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
        swprintf_s(tempcontent, L"%s", (wchar_t*)pContent);

        //�ж��Ƿ���ת����Ϣ
        if (StrStrW(tempcontent, L"΢��ת��")) {
            wstring desc = L"�յ�ת����Ϣ,���Զ��տ�";
            wcscpy(msg->content, desc.c_str());

            //�Զ��տ�
            AutoCllectMoney(fullmessgaedata, msg->wxid);

        } else {
            //�ж���Ϣ���� ������ȳ����Ͳ���ʾ��������Ϣ����Ϊ400
            if (wcslen(tempcontent) > MAX_MSG_LEN) {
                wstring desc = L"��Ϣ���ݹ��� �Ѿ�����";
                wcscpy(msg->content, desc.c_str());

            } else {
                //�ж��Ƿ���ת����Ϣ
                wstring desc = L"�յ�����ʵʱλ�á��ļ������ӵ�������Ϣ,�����ֻ��ϲ鿴";
                wcscpy(msg->content, desc.c_str());
            }
        }

    } else {
        switch (msgType) {
        case kCommandTypeImage: { // ����ͼƬ��Ϣ
            //swprintf_s(msg->content, L"%s", L"�յ�ͼƬ��Ϣ,�����ֻ��ϲ鿴");
            time_t rawtime;
            time(&rawtime);

            swprintf_s(msg->content, L"%s", "");

            g_pic_list.push_back(rawtime);
            g_pic_map.insert(std::make_pair(rawtime, msg));
            return;
        }

        case kCommandTypeCard:
            swprintf_s(msg->content, L"%s", L"�յ���Ƭ��Ϣ,���Զ���Ӻ���");
            //�Զ���Ӻ���
            AutoAddCardUser(fullmessgaedata);
            break;

        case kCommandTypeFriendAck:
            swprintf_s(msg->content, L"%s", L"�յ���������,���Զ�ͨ��");
            //�Զ�ͨ����������
            AutoAgreeUserRequest(fullmessgaedata);
            break;

        case kCommandTypeSysMsg: { // ϵͳ��Ϣ
            wchar_t tempbuff[0x1000];
            LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
            swprintf_s(tempbuff, L"%s", (wchar_t*)pContent);

            // �����ﴦ�����Ⱥ����Ϣ
            if ((StrStrW(tempbuff, L"�Ƴ���Ⱥ��") || StrStrW(tempbuff, L"������Ⱥ��"))) {
                wcscpy_s(msg->content, wcslen(tempbuff) + 1, tempbuff);

            } else {
                memcpy(msg->content,  L"�յ������ϵͳ��Ϣ,�����ֻ��ϲ鿴", sizeof(L"�յ������ϵͳ��Ϣ,�����ֻ��ϲ鿴"));
            }

            break;
        }

        default: { //������������Ϣ֮��
            wchar_t tempbuff[8192];
            LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
            swprintf_s(tempbuff, 8192, L"%s", (wchar_t*)pContent); //�޸Ļ�ȡ����

            //�ж���Ϣ���� ������ȳ����Ͳ���ʾ��������Ϣ����Ϊ400
            if (wcslen(tempbuff) > MAX_MSG_LEN) {
                memcpy(msg->content, L"��Ϣ���ݹ������Ѿ�����", sizeof(L"��Ϣ���ݹ��� �Ѿ�����"));

            } else {
                swprintf_s(msg->content, L"%s", (wchar_t*)pContent);
            }

            break;
        }
        }
    }

    /*
    else if (isRadioMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"�յ���Ƶ��Ϣ,�����ֻ��ϲ鿴");

    } else if (isVoiceMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"�յ�������Ϣ,�����ֻ��ϲ鿴");

    } else if (isLocationMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"�յ�λ����Ϣ,�����ֻ��ϲ鿴");

    } else if (isExpressionMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"�յ�������Ϣ,�����ֻ��ϲ鿴");
    }
    */

    //���͵����ƶ�
    HWND hWnd = FindWindow(NULL, TEXT("΢������"));

    if (hWnd == NULL) {
        LogError("δ���ҵ�΢�����ִ���");
        return;
    }

    COPYDATASTRUCT chatmsg;
    chatmsg.dwData = WM_ShowChatRecord;//����һ����ֵ, ������������־��
    chatmsg.cbData = sizeof(Message);// strlen(szSendBuf);//�����͵����ݵĳ�
    chatmsg.lpData = msg;// szSendBuf;//�����͵����ݵ���ʼ��ַ(����ΪNULL)
    SendMessage(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&chatmsg);
    LogInfo("receive wx msg,from:{},to:{},sessionType:{},content:{}", wstring2string(msg->msgSender), wstring2string(msg->wxid), wstring2string(msg->source), wstring2string(msg->content));
}


//************************************************************
// ��������: GetMsgByAddress
// ����˵��: �ӵ�ַ�л�ȡ��Ϣ����
// ��    ��: GuiShou
// ʱ    ��: 2019/7/6
// ��    ��: DWORD memAddress  Ŀ���ַ
// �� �� ֵ: LPCWSTR	��Ϣ����
//************************************************************
std::wstring GetMsgByAddress(DWORD memAddress) {
    wstring tmp;
    DWORD msgLength = *(DWORD*)(memAddress + 4);

    if (msgLength > 0) {
        WCHAR* msg = new WCHAR[msgLength + 1] { 0 };
        wmemcpy_s(msg, msgLength + 1, (WCHAR*)(*(DWORD*)memAddress), msgLength + 1);
        tmp = msg;
        delete[]msg;
    }

    return  tmp;
}


