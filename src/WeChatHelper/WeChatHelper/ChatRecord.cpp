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


BOOL g_AutoChat = FALSE;	//是否自动聊天
BOOL isSendTuLing = FALSE;	//是否已经发给了图灵机器人
BOOL isText = TRUE;			//是否是文字消息
wchar_t tempwxid[50] = { 0 };	//存放微信ID

DWORD r_esp = 0;
DWORD r_eax = 0;

CHAR originalCode[5] = { 0 };
//参数的偏移
DWORD dwParam = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessageParam;

//计算需要HOOK的地址
DWORD dwHookAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessage - 5;

//返回地址
DWORD RetAddr = dwHookAddr + 5;

enum WxCommandType {
    kCommandTypeText = 0x01,
    kCommandTypeImage = 0x03,
    kCommandTypeVoice = 0x22,
    kCommandTypeCard = 0x2A,
    kCommandTypeShortVideo = 0x3E, // 小视频
    kCommandTypeVideo = 0x2B, // 视频
    kCommandTypeEmotion = 0x2F, // 表情
    kCommandTypeLocation = 0x30, // 位置
    kCommandTypeSharelinkFile = 0x31, // 共享实时位置、文件、转账、链接

    kCommandTypeRecallMsg = 0x2712, // 撤回消息

    kCommandTypeVoipMsg = 0x32,
    kCommandTypeVoipNotify = 0x34, //VOIPNOTIFY
    kCommandTypeVoipInvite = 0x35, // VOIPINVITE,

    kCommandTypeFriendAck = 0x25,
    kCommandTypeFriendPossible = 0x28,

    kCommandTypeWeChatInit = 0x33,

    kCommandTypeSysNotice = 0x270F, // 系统通知
    kCommandTypeSysMsg = 0x2710, // 红包、系统消息
};

//************************************************************
// 函数名称: HookChatRecord
// 函数说明: HOOK聊天记录
// 作    者: GuiShou
// 时    间: 2019/7/6
// 参    数: void
// 返 回 值: void
//************************************************************
void HookChatRecord() {
    //组装数据
    BYTE bJmpCode[5] = { 0xE9 };
    *(DWORD*)&bJmpCode[1] = (DWORD)RecieveWxMesage - dwHookAddr - 5;

    //保存当前位置的指令,在unhook的时候使用。
    ReadProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, originalCode, 5, 0);

    //覆盖指令 B9 E8CF895C //mov ecx,0x5C89CFE8
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, bJmpCode, 5, 0);
}



//************************************************************
// 函数名称: RecieveMesage
// 函数说明: 接收消息
// 作    者: GuiShou
// 时    间: 2019/7/6
// 参    数: void
// 返 回 值: void
//************************************************************
__declspec(naked) void RecieveWxMesage() {
    //保存现场
    __asm {
        //补充被覆盖的代码
        //5B950573  |.  B9 E8CF895C           mov ecx,WeChatWi.5C89CFE8
        //mov ecx,0x5C89CFE8
        mov ecx, dwParam

        //提取esp寄存器内容，放在一个变量中
        mov r_esp, esp
        mov r_eax, eax

        pushad
        pushfd
    }
    SendWxMessage();

    //恢复现场
    __asm {
        popfd
        popad
        //跳回被HOOK指令的下一条指令
        jmp RetAddr
    }
}

wstring GetMsgTypeStr(int msg_type, bool& is_other) {
    switch (msg_type) {
    case kCommandTypeText:
        return L"文字";

    case kCommandTypeImage:
        return L"图片";

    case kCommandTypeVoice:
        return  L"语音";

    case kCommandTypeFriendAck:
        return L"好友确认";

    case kCommandTypeFriendPossible:
        is_other = true;
        return L"POSSIBLEFRIEND_MSG";

    case kCommandTypeCard:
        return L"名片";

    case kCommandTypeVideo:
        return L"视频";

    case kCommandTypeEmotion:
        //石头剪刀布
        return L"表情";

    case kCommandTypeLocation:
        return L"位置";

    case kCommandTypeSharelinkFile:
        //共享实时位置
        //文件
        //转账
        //链接
        //收款
        //is_other = true;
        return L"共享实时位置、文件、转账、链接";

    case kCommandTypeVoipMsg:
        is_other = true;
        return  L"VOIPMSG";

    case kCommandTypeWeChatInit:
        is_other = true;
        return L"微信初始化";

    case kCommandTypeVoipNotify:
        is_other = true;
        return  L"VOIPNOTIFY";

    case kCommandTypeVoipInvite:
        is_other = true;
        return L"VOIPINVITE";

    case kCommandTypeShortVideo:
        return L"小视频";

    case kCommandTypeSysNotice:
        is_other = true;
        return L"SYSNOTICE";

    case kCommandTypeSysMsg:
        //系统消息
        //红包
        return L"红包、系统消息";

    case kCommandTypeRecallMsg:
        return L"撤回消息";

    default:
        is_other = true;
        return L"未知";
    }
}

//************************************************************
// 函数名称: SendMessage
// 函数说明: 将接收到的消息发送给客户端
// 作    者: GuiShou
// 时    间: 2019/7/6
// 参    数: void
// 返 回 值: void
//************************************************************
void SendWxMessage() {
    Message* msg = new Message;
    ::memset(msg, 0, sizeof(msg));

    // 信息块的位置
    DWORD** msgAddress = (DWORD**)r_esp;
    // 消息类型
    DWORD msgType = *((DWORD*)(**msgAddress + 0x30));
    msg->msgType = msgType;

    // 微信ID/群ID
    LPVOID pWxid = *((LPVOID*)(**msgAddress + 0x40));
    swprintf_s(msg->wxid, L"%s", (wchar_t*)pWxid);

    // 消息内容
    wstring fullmessgaedata = GetMsgByAddress(**msgAddress + 0x68);	//完整的消息内容
    //判断消息来源是群消息还是好友消息
    wstring msgSource2 = L"<msgsource />\n";
    wstring msgSource = L"";
    msgSource.append(GetMsgByAddress(**msgAddress + 0x168));

    //LogInfo("receive msg,msgType:{},desc:{},content:{},", msgType, wstring2string(msg->type), wstring2string(fullmessgaedata));

    // 消息类型文本描述
    bool is_other_msg = false;
    wstring desc = GetMsgTypeStr(msg->msgType, is_other_msg);
    memcpy(msg->type, desc.c_str(), desc.length());

    //好友消息
    if (!msgSource.empty() && msgSource.length() <= msgSource2.length()) {
        memcpy(msg->source, L"好友消息", sizeof(L"好友消息"));
        memcpy(msg->msgSender, L"NULL", sizeof(L"NULL"));

    } else {
        // 群消息
        memcpy(msg->source, L"群消息", sizeof(L"群消息"));

        // 显示消息发送者
        LPVOID pSender = *((LPVOID*)(**msgAddress + 0x114));
        swprintf_s(msg->msgSender, L"%s", (wchar_t*)pSender);
    }

    // 显示消息内容  过滤无法显示的消息 防止奔溃
    if (StrStrW(msg->wxid, L"gh")) {
        //如果微信ID为gh_3dfda90e39d6 说明是收款消息
        if ((StrCmpW(msg->wxid, L"gh_3dfda90e39d6") == 0)) {
            wstring desc = L"微信收款到账";
            wcscpy(msg->content, desc.c_str());
            msg->isMoney = TRUE;

        } else {
            //如果微信ID中带有gh 说明是公众号
            wstring desc = L"公众号发来推文,请在手机上查看";
            wcscpy(msg->content, desc.c_str());
        }

    } else if (is_other_msg) {
        //取出消息内容
        wchar_t tempcontent[0x10000] = { 0 };
        LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
        swprintf_s(tempcontent, L"%s", (wchar_t*)pContent);

        //判断是否是转账消息
        if (StrStrW(tempcontent, L"微信转账")) {
            wstring desc = L"收到转账消息,已自动收款";
            wcscpy(msg->content, desc.c_str());

            //自动收款
            AutoCllectMoney(fullmessgaedata, msg->wxid);

        } else {
            //判断消息长度 如果长度超过就不显示，设置消息长度为400
            if (wcslen(tempcontent) > MAX_MSG_LEN) {
                wstring desc = L"消息内容过长 已经过滤";
                wcscpy(msg->content, desc.c_str());

            } else {
                //判断是否是转账消息
                wstring desc = L"收到共享实时位置、文件、链接等其他消息,请在手机上查看";
                wcscpy(msg->content, desc.c_str());
            }
        }

    } else {
        switch (msgType) {
        case kCommandTypeImage: { // 过滤图片消息
            //swprintf_s(msg->content, L"%s", L"收到图片消息,请在手机上查看");
            time_t rawtime;
            time(&rawtime);

            swprintf_s(msg->content, L"%s", "");

            g_pic_list.push_back(rawtime);
            g_pic_map.insert(std::make_pair(rawtime, msg));
            return;
        }

        case kCommandTypeCard:
            swprintf_s(msg->content, L"%s", L"收到名片消息,已自动添加好友");
            //自动添加好友
            AutoAddCardUser(fullmessgaedata);
            break;

        case kCommandTypeFriendAck:
            swprintf_s(msg->content, L"%s", L"收到好友请求,已自动通过");
            //自动通过好友请求
            AutoAgreeUserRequest(fullmessgaedata);
            break;

        case kCommandTypeSysMsg: { // 系统消息
            wchar_t tempbuff[0x1000];
            LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
            swprintf_s(tempbuff, L"%s", (wchar_t*)pContent);

            // 在这里处理加入群聊消息
            if ((StrStrW(tempbuff, L"移出了群聊") || StrStrW(tempbuff, L"加入了群聊"))) {
                wcscpy_s(msg->content, wcslen(tempbuff) + 1, tempbuff);

            } else {
                memcpy(msg->content,  L"收到红包或系统消息,请在手机上查看", sizeof(L"收到红包或系统消息,请在手机上查看"));
            }

            break;
        }

        default: { //过滤完所有消息之后
            wchar_t tempbuff[8192];
            LPVOID pContent = *((LPVOID*)(**msgAddress + 0x68));
            swprintf_s(tempbuff, 8192, L"%s", (wchar_t*)pContent); //修改获取长度

            //判断消息长度 如果长度超过就不显示，设置消息长度为400
            if (wcslen(tempbuff) > MAX_MSG_LEN) {
                memcpy(msg->content, L"消息内容过长，已经过滤", sizeof(L"消息内容过长 已经过滤"));

            } else {
                swprintf_s(msg->content, L"%s", (wchar_t*)pContent);
            }

            break;
        }
        }
    }

    /*
    else if (isRadioMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"收到视频消息,请在手机上查看");

    } else if (isVoiceMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"收到语音消息,请在手机上查看");

    } else if (isLocationMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"收到位置消息,请在手机上查看");

    } else if (isExpressionMessage == TRUE) {
        swprintf_s(msg->content, L"%s", L"收到表情消息,请在手机上查看");
    }
    */

    //发送到控制端
    HWND hWnd = FindWindow(NULL, TEXT("微信助手"));

    if (hWnd == NULL) {
        LogError("未查找到微信助手窗口");
        return;
    }

    COPYDATASTRUCT chatmsg;
    chatmsg.dwData = WM_ShowChatRecord;//保存一个数值, 可以用来作标志等
    chatmsg.cbData = sizeof(Message);// strlen(szSendBuf);//待发送的数据的长
    chatmsg.lpData = msg;// szSendBuf;//待发送的数据的起始地址(可以为NULL)
    SendMessage(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&chatmsg);
    LogInfo("receive wx msg,from:{},to:{},sessionType:{},content:{}", wstring2string(msg->msgSender), wstring2string(msg->wxid), wstring2string(msg->source), wstring2string(msg->content));
}


//************************************************************
// 函数名称: GetMsgByAddress
// 函数说明: 从地址中获取消息内容
// 作    者: GuiShou
// 时    间: 2019/7/6
// 参    数: DWORD memAddress  目标地址
// 返 回 值: LPCWSTR	消息内容
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


