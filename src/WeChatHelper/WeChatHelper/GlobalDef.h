#pragma once
#include <list>
#include <string>
#include <map>
#include <wtypes.h>
//#include "ChatRecord.h"

#define   MAX_MSG_LEN    4 * 1000
//消息结构体
struct Message {
    int     msgType;        //消息类型
    wchar_t type[10];		//消息类型
    wchar_t source[20];		//消息来源
    wchar_t wxid[40];		//微信ID/群ID
    wchar_t msgSender[40];	//消息发送者
    wchar_t content[MAX_MSG_LEN];	//消息内容.
    BOOL isMoney = FALSE;	//是否是收款消息.
};

extern std::list<int64_t>  g_pic_list;
extern std::map<int64_t, Message*>  g_pic_map;

