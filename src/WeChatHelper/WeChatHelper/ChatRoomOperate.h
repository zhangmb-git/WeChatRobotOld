#pragma once
#include <string>
void SetWxRoomAnnouncement(wchar_t* chatroomwxid, const wchar_t* Announcement);	//����Ⱥ����
void QuitChatRoom(wchar_t* chatroomwxid);	//�˳�Ⱥ��
void AddGroupMember(wchar_t* chatroomwxid, wchar_t* wxid);	//���Ⱥ��Ա
void ShowChatRoomUser(wchar_t* chatroomwxid);	//��ʾȺ���г�Ա
void ShowChatRoomMember(wchar_t* chatroomwxid, wchar_t* wxid);	//��ʾȺ��Ա
void GetUserInfoByWxid(wchar_t* userwxid);		//ͨ��΢��ID��ȡ�û���Ϣ

void SetRoomName(wchar_t* roomwxid, wchar_t* roomname);	//�޸�Ⱥ����

void DelRoomMember(wchar_t* roomid, wchar_t* memberwxid);	//ɾ��Ⱥ��Ա
//���Ͱ�����Ϣ
void SendRoomAtMsg(std::wstring  chatroomid, std::wstring memberwxid, std::wstring membernickname, std::wstring msg);