//
//  public_define.h
//  im-server-mac-new
//
//  Created by luoning on 14-12-24.
//  Copyright (c) 2014年 luoning. All rights reserved.
//


#pragma  once
#include <iostream>
#include <set>
#include <stdint.h>
#include <unordered_map>
#include <map>
#include <list>
#include <string>
#include <atomic>
#include <windows.h>
#include "../../../Network/protocol/common_def.h"

//======================================================
#define MAX_MSG_LEN     4*1000

//消息结构体.
typedef struct {
    wchar_t wxid[40];
    wchar_t content[MAX_MSG_LEN];
} MessageStruct;

//个人信息的结构体
struct tagInformation {
    std::string wxid;		//微信ID.
    std::string wxcount;	//微信账号.
    std::string nickname;	//微信昵称.
    std::string wxsex;		//性别.
    std::string phone;		//手机号.
    std::string device;		//登陆设备.
    std::string nation;		//国籍.
    std::string province;	//省份.
    std::string city;		//城市.
    std::string header;	//头像.
};

//消息结构体.
struct Message {
    int     msgType;        //消息类型.
    wchar_t type[10];		//消息类型.
    wchar_t source[20];		//消息来源.
    wchar_t wxid[40];		//微信ID/群ID.
    wchar_t msgSender[40];	//消息发送者.
    wchar_t content[MAX_MSG_LEN];  //消息内容.
};

//房间成员信息
struct   RoomMemberInfo {
    wchar_t chatroomid[50];
    wchar_t wxid[50];
};


//聊天房间成员信息
struct ChatRoomMemberInfo {
    wchar_t GroupId[50];
    wchar_t UserId[50];
    wchar_t UserNumber[50];
    wchar_t UserNickName[50];
};

//名片信息
struct XmlCardMessage {
    wchar_t RecverWxid[50];		//接收者的微信ID
    wchar_t SendWxid[50];		//需要发送的微信ID
    wchar_t NickName[50];		//昵称
};


typedef std::unordered_map<std::string, std::string>  MAP_ID_TO_NAME;
struct  WxData {
    std::atomic_bool    bGetInfoOK = false;
    tagInformation  tPrivateInfo;
    std::unordered_map<std::string, std::string>  mapFriendList;
    std::unordered_map<std::string, MAP_ID_TO_NAME> mapGroupMemberInfo;
    std::unordered_map<std::string, std::atomic_bool>mapGroupMemberStatus;

};

extern  WxData  g_WxData;


//胖猫http响应
struct tagHttpResp {
    int   respType = 0;
    std::string respText = "";
    std::string respUrl = "";
};

//昵称模型参数
struct tagNickNameParam {
    tagHttpResp  tHttpResp;
    std::string  strNickName = "";
    std::string  strWxGroupID = "";
    std::string  strWxUserID = "";
};

//提问模型
struct tagTaskParam {
    int    msgType;        //消息类型
    int    incrementalID;  //用户维度的递增消息编码
    int    msgSource;      //1.私聊 2：群聊.
    int    sendTime;       //发送事件戳.
    std::string msg;
    std::string msgSendId;
    std::string groupId;
};


struct AtMsg {
    wchar_t chatroomid[50] = { 0 };
    wchar_t memberwxid[50] = { 0 };
    wchar_t membernickname[50] = { 0 };
    wchar_t msgcontent[100] = { 0 };
};



enum E_Msg_Type  {
    //客户端和服务端通讯消息
    E_Msg_Login = 0,
    E_Msg_ShowQrPicture = 1,
    E_Msg_Logout = 2,
    E_Msg_GetFriendList = 3,
    E_Msg_ShowChatRecord = 4,
    E_Msg_SendTextMessage = 5,
    E_Msg_SendFileMessage = 6,
    E_Msg_GetInformation = 7,
    E_Msg_SendImageMessage = 8,
    E_Msg_SetRoomAnnouncement = 9,
    E_Msg_DeleteUser = 10,
    E_Msg_QuitChatRoom = 11,
    E_Msg_AddGroupMember = 12,
    E_Msg_SendXmlCard = 13,
    E_Msg_ShowChatRoomMembers = 14,
    E_Msg_ShowChatRoomMembersDone = 15,
    E_Msg_DecryptDatabase = 16,
    E_Msg_AddUser = 17,
    E_Msg_SetRoomName = 18,
    E_Msg_AutoChat = 19,
    E_Msg_CancleAutoChat = 20,
    E_Msg_AlreadyLogin = 21,
    E_Msg_SendAtMsg = 22,
    E_Msg_DelRoomMember = 23,
    E_Msg_OpenUrl = 24,
    E_Msg_SaveFriendList = 25,
    E_Msg_ShowChatRoomMember = 26

};

