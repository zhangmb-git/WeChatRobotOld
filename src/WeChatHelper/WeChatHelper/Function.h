#pragma once
void GetInformation();	//��ȡ������Ϣ
wchar_t * UTF8ToUnicode(const char* str); //��UTF8����תΪUnicode(΢��Ĭ�ϱ���ΪUTF8)
void AddWxUser(wchar_t* wxid, wchar_t* msg);	//��Ӻ���
void AntiRevoke();	//������
void OpenUrl(wchar_t * Url);	//��΢�������


