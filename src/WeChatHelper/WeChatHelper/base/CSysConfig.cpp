
#include "CSysConfig.h"
#include "IniOperation.h"
#include "util.h"
#include "ZLogger.h"

namespace module {
    CSysConfig* getSysConfigModule() {
        static CSysConfig sysConfig;
        return &sysConfig;
    }
}

CSysConfig::CSysConfig() {
    _loadData();
}

CString getAppPath() {
    static CString g_sDllPath = _T("");

    if (g_sDllPath.IsEmpty()) {
        TCHAR	buffer[MAX_PATH];
        ZeroMemory(buffer, sizeof(TCHAR)* MAX_PATH);
        HMODULE h = GetModuleHandle(NULL);
        ::GetModuleFileName(h, buffer, MAX_PATH);
        ::PathRemoveFileSpec(buffer);
        g_sDllPath = buffer;
        g_sDllPath += _T("\\");
    }

    return g_sDllPath;
}

void CSysConfig::_loadData() {

    //获取临时文件夹目录
    char temppath[MAX_PATH] = { 0 };
    GetTempPathA(MAX_PATH, temppath);
    //CString szAppPath = getAppPath();
    //std::string  strAppPath = wstring2string(szAppPath.GetString());
    char imagedir[20] = { "WeChatRecordImages" };

    //拼接目录
    char WeChatImgPath[MAX_PATH] = { 0 };
    sprintf_s(WeChatImgPath, "%s%s\\", temppath, imagedir);
    //创建目录存放图片
    CreatePath(WeChatImgPath);

    m_imgPath = WeChatImgPath;
    return;
}

std::string CSysConfig::getImgPath() {
    return  m_imgPath;
}

void CSysConfig::setKeyData(const CString& section, const CString& key, const CString& value) {

    CString iniPath = getAppPath() + _T("config.ini");
    CIniWriter  iniWriter(iniPath);
    iniWriter.WriteString(section, key, value);
    return;
}

