#include "stdafx.h"
#include "CSaveImage.h"
#include <atlconv.h>
#include <string>
#include <direct.h> //_mkdir������ͷ�ļ�
#include <io.h>     //_access������ͷ�ļ�
#include <iostream>
#include <cstdio>
#include <ctime>
#include <stdio.h>
#include "base/Zlogger.h"
#include "GlobalDef.h"
#include "base/util.h"
#include "base/CSysConfig.h"
#include <chrono>

#include <unordered_map>


using namespace std;


const BYTE  jpgHead[3] = { 0xFF, 0xD8, 0xFF };
const BYTE  pngHead[4] = { 0x89, 0x50, 0x4E, 0x47 };
const BYTE  gifHead[4] = { 0x47, 0x49, 0x46, 0x38 };



//���ݵ�ԭ����
//DWORD SaveImageAddress =  (DWORD)GetModuleHandle(L"WeChatWin.dll") + WxGetPicAddr;
DWORD SaveImageAddressBackAddress = (DWORD)GetModuleHandle(L"WeChatWin.dll") + WxGetPicAddr + 5;	//���ص�ַ

//�Լ�����
DWORD ImageDataLen = 0;
BYTE* ImageData = nullptr;

//unordered_map<int64_t, int64_t> g_pic_saved;

void HookSaveImages(DWORD dwHookOffset) {
    DWORD dwBaseAddress = (DWORD)GetModuleHandle(TEXT("WeChatWin.dll"));
    ////��Ҫhook�ĵ�ַ
    DWORD  SaveImageAddress = dwBaseAddress + dwHookOffset;

    ////���صĵ�ַ
    //DWORD SaveImageAddressBackAddress = SaveImageAddress + 5;

    //��װ��ת����
    BYTE jumpCode[5] = { 0 };
    jumpCode[0] = 0xE9;

    //����ƫ��
    *(DWORD*)& jumpCode[1] = (DWORD)FnSaveImages - SaveImageAddress - 5;

    // ������ǰ���������ڻ�ԭ
    DWORD OldProtext = 0;

    // ��ΪҪ�������д�����ݣ�����Ϊ������ǲ���д�ģ�������Ҫ�޸�����
    VirtualProtect((LPVOID)SaveImageAddress, 5, PAGE_EXECUTE_READWRITE, &OldProtext);

    //д���Լ��Ĵ���
    memcpy((void*)SaveImageAddress, jumpCode, 5);

    // ִ�����˲���֮����Ҫ���л�ԭ
    VirtualProtect((LPVOID)SaveImageAddress, 5, OldProtext, &OldProtext);
}


__declspec(naked) void FnSaveImages() {
    __asm {
        mov ebx, dword ptr ss : [ebp - 0x4];
        mov ImageData, ebx;
        mov ImageDataLen, ecx;       //esp + 0x4;
        pushad;
        pushfd;
    }

    //���ý�����Ϣ�ĺ���
    FnSaveImagesCore();

    //�ָ��ֳ�
    __asm {
        popfd
        popad
        //���ر�HOOKָ�����һ��ָ��
        jmp SaveImageAddressBackAddress;
    }
}


void FnSaveImagesCore() {
    //���ͼƬ���ȴ���1KB�򱣴�

    //��ȡ��ʱ�ļ���Ŀ¼
    std::string imgDir = module::getSysConfigModule()->getImgPath();

    if (g_pic_list.empty()) {
        LogWarn("save image error,g_pic_list empty");
        return;
    }

    int64_t rawTime = g_pic_list.front();
    g_pic_list.pop_front();

    //time_t cur = time(nullptr);
    //bool is_time_out = ::abs(g_last_call_time - cur) > 1000;

    //// ��һ��������ͼ���ڶ��Ųű���
    //if (is_time_out || g_pic_saved.find(rawTime) != g_pic_saved.end()) {
    //    g_pic_list.pop_front();
    //    g_pic_saved.erase(rawTime);

    //} else {
    //    g_pic_saved[rawTime] = rawTime;
    //    return;
    //}

    // С��1KB�ģ�ֱ�Ӷ�������ֹת���쳣
    if (ImageDataLen < 1024) {
        LogWarn("image too small, {} < 1024", ImageDataLen);
        return;
    }

    std::string  strExt = ".jpg";

    if (memcmp(ImageData, jpgHead, 3) == 0) {
        strExt = ".jpg";

    } else if (memcmp(ImageData, pngHead, 4) == 0) {
        strExt = ".png";
    }

    //����ͼƬ
    std::string  strImgPath = CreateFileWithCurrentTime(imgDir.c_str(), strExt.c_str(), ImageData, ImageDataLen, rawTime);

    //������Ϣ
    if (strImgPath.empty()) {
        LogError("image path empty");
        return;
    }

    SendPicMsg(rawTime, strImgPath);

}


std::string CreateFileWithCurrentTime(const char* path, const char* ext, BYTE* buf, DWORD len, int64_t pic_id) {

    struct tm ptminfo;
    localtime_s(&ptminfo, &pic_id);
    char currenttime[30] = { 0 };
    sprintf_s(currenttime, "%02d%02d%02d%02d%02d%02d", ptminfo.tm_year + 1900,
              ptminfo.tm_mon + 1, ptminfo.tm_mday, ptminfo.tm_hour, ptminfo.tm_min, ptminfo.tm_sec);

    std::string imgDir = module::getSysConfigModule()->getImgPath();

    //ƴ��������ͼƬ·��
    char imagepath[MAX_PATH] = { 0 };
    sprintf_s(imagepath, "%s%s%s", imgDir.c_str(), currenttime, ext);


    //���ļ�д��TempĿ¼��
    HANDLE hFile = CreateFileA(imagepath, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == NULL) {
        //MessageBoxA(NULL, "����ͼƬ�ļ�ʧ��", "����", 0);
        LogError("����ͼƬ�ļ�ʧ��");
        return "";
    }

    DWORD dwRead = 0;

    if (WriteFile(hFile, (LPCVOID)buf, len, &dwRead, NULL) == 0) {
        LogError("д��ͼƬ�ļ�ʧ��");
        //MessageBoxA(NULL, "д��ͼƬ�ļ�ʧ��", "����", 0);
    }

    CloseHandle(hFile);
    return std::string(imagepath);
}


void SendPicMsg(int64_t pic_id, std::string image_path) {

    //��ȡ��ǰʱ����Ϊ�ļ���
    if (g_pic_map.size() <= 0) {
        LogError("g_pic_map  empty");
        return;
    }

    if (g_pic_map.find(pic_id) == g_pic_map.end()) {
        LogError("pic not found in map ,pic_id:{},map_size:{}", pic_id, g_pic_map.size());
        return;
    }

    Message* msg = g_pic_map[pic_id];
    swprintf_s(msg->content, L"%s", string2wstring(image_path).c_str());

    LogInfo("send  save img  msg,from:{},to:{},msg:{}", wstring2string(msg->msgSender), wstring2string(msg->source), wstring2string(msg->content));
    //���͵����ƶ�
    HWND hWnd = FindWindow(NULL, TEXT("΢������"));

    if (hWnd == NULL) {
        LogError("δ���ҵ�΢�����ִ���");
        return;
    }

    COPYDATASTRUCT chatmsg;
    chatmsg.dwData = WM_ShowChatRecord;//����һ����ֵ, ������������־��
    chatmsg.cbData = sizeof(Message);// strlen(szSendBuf);//�����͵����ݵĳ�
    chatmsg.lpData = msg;
    SendMessage(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&chatmsg);

    if (msg) {
        g_pic_map.erase(pic_id);
        delete  msg;
        msg = nullptr;
    }

    return;
}


