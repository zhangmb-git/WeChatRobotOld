#pragma once
#include "stdafx.h"
#include "ChatRecord.h"



//���յ��ı���Ϣ�ṹ��
struct MessageStruct {
    wchar_t wxid[40];
    //wchar_t content[MAX_MSG_LEN];
};


void InitWindow(HMODULE hModule);	//��ʼ������
void RegisterWindow(HMODULE hModule);		//ע�ᴰ��
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);	//���ڻص�