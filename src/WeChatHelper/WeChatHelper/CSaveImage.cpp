#include "stdafx.h"
#include "CSaveImage.h"
#include <atlconv.h>
#include <string>
#include <direct.h> //_mkdir函数的头文件
#include <io.h>     //_access函数的头文件
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



//备份的原数据
//DWORD SaveImageAddress =  (DWORD)GetModuleHandle(L"WeChatWin.dll") + WxGetPicAddr;
DWORD SaveImageAddressBackAddress = (DWORD)GetModuleHandle(L"WeChatWin.dll") + WxGetPicAddr + 5;	//返回地址

//自己生成
DWORD ImageDataLen = 0;
BYTE* ImageData = nullptr;

//unordered_map<int64_t, int64_t> g_pic_saved;

void HookSaveImages(DWORD dwHookOffset) {
    DWORD dwBaseAddress = (DWORD)GetModuleHandle(TEXT("WeChatWin.dll"));
    ////需要hook的地址
    DWORD  SaveImageAddress = dwBaseAddress + dwHookOffset;

    ////跳回的地址
    //DWORD SaveImageAddressBackAddress = SaveImageAddress + 5;

    //组装跳转数据
    BYTE jumpCode[5] = { 0 };
    jumpCode[0] = 0xE9;

    //计算偏移
    *(DWORD*)& jumpCode[1] = (DWORD)FnSaveImages - SaveImageAddress - 5;

    // 保存以前的属性用于还原
    DWORD OldProtext = 0;

    // 因为要往代码段写入数据，又因为代码段是不可写的，所以需要修改属性
    VirtualProtect((LPVOID)SaveImageAddress, 5, PAGE_EXECUTE_READWRITE, &OldProtext);

    //写入自己的代码
    memcpy((void*)SaveImageAddress, jumpCode, 5);

    // 执行完了操作之后需要进行还原
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

    //调用接收消息的函数
    FnSaveImagesCore();

    //恢复现场
    __asm {
        popfd
        popad
        //跳回被HOOK指令的下一条指令
        jmp SaveImageAddressBackAddress;
    }
}


void FnSaveImagesCore() {
    //如果图片长度大于1KB则保存

    //获取临时文件夹目录
    std::string imgDir = module::getSysConfigModule()->getImgPath();

    if (g_pic_list.empty()) {
        LogWarn("save image error,g_pic_list empty");
        return;
    }

    int64_t rawTime = g_pic_list.front();
    g_pic_list.pop_front();

    //time_t cur = time(nullptr);
    //bool is_time_out = ::abs(g_last_call_time - cur) > 1000;

    //// 第一张是缩略图，第二张才保存
    //if (is_time_out || g_pic_saved.find(rawTime) != g_pic_saved.end()) {
    //    g_pic_list.pop_front();
    //    g_pic_saved.erase(rawTime);

    //} else {
    //    g_pic_saved[rawTime] = rawTime;
    //    return;
    //}

    // 小于1KB的，直接丢弃。防止转发异常
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

    //保存图片
    std::string  strImgPath = CreateFileWithCurrentTime(imgDir.c_str(), strExt.c_str(), ImageData, ImageDataLen, rawTime);

    //发送消息
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

    //拼接完整的图片路径
    char imagepath[MAX_PATH] = { 0 };
    sprintf_s(imagepath, "%s%s%s", imgDir.c_str(), currenttime, ext);


    //将文件写到Temp目录下
    HANDLE hFile = CreateFileA(imagepath, GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == NULL) {
        //MessageBoxA(NULL, "创建图片文件失败", "错误", 0);
        LogError("创建图片文件失败");
        return "";
    }

    DWORD dwRead = 0;

    if (WriteFile(hFile, (LPCVOID)buf, len, &dwRead, NULL) == 0) {
        LogError("写入图片文件失败");
        //MessageBoxA(NULL, "写入图片文件失败", "错误", 0);
    }

    CloseHandle(hFile);
    return std::string(imagepath);
}


void SendPicMsg(int64_t pic_id, std::string image_path) {

    //获取当前时间作为文件名
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
    //发送到控制端
    HWND hWnd = FindWindow(NULL, TEXT("微信助手"));

    if (hWnd == NULL) {
        LogError("未查找到微信助手窗口");
        return;
    }

    COPYDATASTRUCT chatmsg;
    chatmsg.dwData = WM_ShowChatRecord;//保存一个数值, 可以用来作标志等
    chatmsg.cbData = sizeof(Message);// strlen(szSendBuf);//待发送的数据的长
    chatmsg.lpData = msg;
    SendMessage(hWnd, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&chatmsg);

    if (msg) {
        g_pic_map.erase(pic_id);
        delete  msg;
        msg = nullptr;
    }

    return;
}


