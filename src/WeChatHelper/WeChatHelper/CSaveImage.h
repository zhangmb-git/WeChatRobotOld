#pragma once
#include <string>
#include "GlobalDef.h"

using namespace std;

void HookSaveImages(DWORD dwHookOffset);
void FnSaveImages();
void FnSaveImagesCore();
std::string CreateFileWithCurrentTime(const char* path, const char* ext, BYTE* buf, DWORD len, int64_t pic_id);

void AgreeUserRequest(wchar_t* v1, wchar_t* v2);	//ͬ���������
void AutoAgreeUserRequest(wstring msg);	//�Զ�ͬ���������

void CllectMoney(wchar_t* transferid, wchar_t* wxid);	//�տ�
void AutoCllectMoney(wstring msg, wchar_t* wxid);	//�Զ��տ�

void AddCardUser(wchar_t* v1, wchar_t* msg);	//�����Ƭ����
void AutoAddCardUser(wstring msg);			//�Զ������Ƭ����


void ExtractExpression();				//��ȡ����
void HookExtractExpression(DWORD dwHookOffset);//HOOK��ȡ����
void OutputExpression(DWORD dwExpressionAddr);	//�������

void SendPicMsg(int64_t pic_id, std::string image_path);
