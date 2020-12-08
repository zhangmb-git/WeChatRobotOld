/** @file common_def.h
  * @brief common_def
  * @author yingchun.xu
  * @date 2020/8/14
  */

#ifndef _COMMON_DEF_46C302AC_733B_4B43_9277_FC3A0FF193BD_
#define _COMMON_DEF_46C302AC_733B_4B43_9277_FC3A0FF193BD_

#include <cstdint>

//�ͻ��˺ͷ����ͨѶ��Ϣ
enum CommondType {
    /*E_Msg_Login = 0,
    E_Msg_ShowQrPicture = 1,
    E_Msg_Logout = 2,*/

    //kCmdTypeGetFriendListReq = 0x100, // ��������б�
    //kCmdTypeGetFriendListRsp = 0x101,

    kCmdTypeUnknown = 0x0,

    kCmdTypeGetUserNickNameReq = 0x102, // ��ѯ�ǳ�
    kCmdTypeGetUserNickNameRsp = 0x103,

    kCmdTypeMsgData = 0x200, // ������Ϣ
    kCmdTypeMsgAck = 0x201,

    //E_Msg_ShowChatRecord = 4,
    //E_Msg_SendTextMessage = 5,
    //E_Msg_SendFileMessage = 6,
    //E_Msg_GetInformation = 7,
    //E_Msg_SendImageMessage = 8,
    //E_Msg_SetRoomAnnouncement = 9,
    //E_Msg_DeleteUser = 10,
    //E_Msg_QuitChatRoom = 11,
    //E_Msg_AddGroupMember = 12,
    //E_Msg_SendXmlCard = 13,
    //E_Msg_ShowChatRoomMembers = 14,
    //E_Msg_ShowChatRoomMembersDone = 15,
    //E_Msg_DecryptDatabase = 16,
    //E_Msg_AddUser = 17,
    //E_Msg_SetRoomName = 18,
    //E_Msg_AutoChat = 19,
    //E_Msg_CancleAutoChat = 20,
    //E_Msg_AlreadyLogin = 21,
    //E_Msg_SendAtMsg = 22,
    //E_Msg_DelRoomMember = 23,
    //E_Msg_OpenUrl = 24,
    //E_Msg_SaveFriendList = 25,
    //E_Msg_ShowChatRoomMember = 26		//
};

// Э��ͷ������8�ֽ�
struct IMHeader {
    uint32_t len;			// Э���峤��
    uint16_t command_id;	// ����ID����CommondType
    uint16_t seq_;			// ��ţ�Ӧ�Ϳͻ��˵ı���һ�¡�
    uint32_t reserved;		// Ԥ����4�ֽڡ�
};

// Robot �� WeChatHelperͨ�ŵ���Ϣ���Ͷ���
enum IMChatMsgType {
    msg_type_atallmsg = 0,
    msg_type_text = 1,            //�ı�
    msg_type_pic = 3,             //ͼƬ
    msg_type_voice = 34,          //����
    msg_type_friendrequest = 37,  //����ȷ��
    msg_type_video = 43,          //��Ƶ
    msg_type_expression = 47,     //����
    msg_type_location = 48,       //λ��
    msg_type_sharelink_file = 49, //�������� 0x31
    msg_type_radio = 62           //С��Ƶ
};

//������Ϣ
struct IMUserInfo {
    wchar_t user_id[80];
    wchar_t user_number[80];
    wchar_t user_remark[80];
    wchar_t user_nick_name[80];
};

const uint32_t kHeaderLen = sizeof(IMHeader);

#endif//_COMMON_DEF_46C302AC_733B_4B43_9277_FC3A0FF193BD_

