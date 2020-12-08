#pragma once
#include "stdafx.h"
#include "ChatRecord.h"



//接收的文本消息结构体
struct MessageStruct {
    wchar_t wxid[40];
    //wchar_t content[MAX_MSG_LEN];
};


void InitWindow(HMODULE hModule);	//初始化窗口
void RegisterWindow(HMODULE hModule);		//注册窗口
LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);	//窗口回调