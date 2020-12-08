#include "stdafx.h"
#include "CSysConfig.h"
#include "base/util.h"
#include "base/ZLogger.h"
#include "common/IniOperation.h"

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
    CString iniPath = getAppPath() + _T("config.ini");
    CIniReader  iniReader(iniPath);

    //[WX]
    USES_CONVERSION;
    m_pConfig.wxGroup = W2A(iniReader.ReadString(_T("WX"), _T("Group"), _T("")));
    m_pConfig.catKey = W2A(iniReader.ReadString(_T("WX"), _T("CatKey"), _T("")));
    m_pConfig.catUrl = W2A(iniReader.ReadString(_T("WX"), _T("CatUrl"), _T("")));
    m_pConfig.mqUrl = W2A(iniReader.ReadString(_T("WX"), _T("MqUrl"), _T("")));
    m_pConfig.fileDir = W2A(iniReader.ReadString(_T("WX"), _T("FileDIr"), _T("")));
    m_pConfig.imageDir = W2A(iniReader.ReadString(_T("WX"), _T("ImageDIr"), _T("")));
    m_pConfig.catKeyOpen = iniReader.ReadInteger(_T("WX"), _T("CatKeyOpen"), 0);
    m_pConfig.privateOpen = iniReader.ReadInteger(_T("WX"), _T("PrivateOpen"), 1);
    m_pConfig.groupOpen = iniReader.ReadInteger(_T("WX"), _T("GroupOpen"), 1);
    m_pConfig.chatMode = iniReader.ReadInteger(_T("WX"), _T("ChatMode"), 0);

    //[LIMIT]
    m_pConfig.trafficLimit = iniReader.ReadInteger(_T("LIMIT"), _T("TrafficLimit"), 2);
    m_pConfig.trafficInterval = iniReader.ReadInteger(_T("LIMIT"), _T("TrafficInterval"), 3);

    //[PUSH]
    m_pConfig.pushFlag = iniReader.ReadInteger(_T("PUSH"), _T("PushFlag"), 0);
    m_pConfig.pushUrl = W2A(iniReader.ReadString(_T("PUSH"), _T("PushUrl"), _T("")));

    //[TRANSFER]
    m_pConfig.transferFlag = iniReader.ReadInteger(_T("TRANSFER"), _T("TransferFlag"), 0);
    m_pConfig.transferInterval = iniReader.ReadInteger(_T("TRANSFER"), _T("TransferInterval"), 0);
    m_pConfig.imageFlag = iniReader.ReadInteger(_T("TRANSFER"), _T("ImageFlag"), 0);
    std::string  strFromGroup = W2A(iniReader.ReadString(_T("TRANSFER"), _T("FromGroup"), _T("")));
    StringToArr(strFromGroup, m_pConfig.vecFromGroup);
    std::string strToGroup = W2A(iniReader.ReadString(_T("TRANSFER"), _T("ToGroup"), _T("")));
    StringToArr(strToGroup, m_pConfig.vecToGroup);

    //[server]
    m_pConfig.serverIP = W2A(iniReader.ReadString(_T("SERVER"), _T("ServerIP"), _T("")));
    m_pConfig.port = iniReader.ReadInteger(_T("SERVER"), _T("Port"), 0);
    LogInfo("\n catKeyOPen:{}\n privateOpen:{}\n groupOpen:{}\n chatMode:{}\n catUrl£º{}",
            m_pConfig.catKeyOpen, m_pConfig.privateOpen, m_pConfig.groupOpen, m_pConfig.chatMode, m_pConfig.catUrl);
}

tagConfig* CSysConfig::getSysConfig() {
    return  &m_pConfig;
}

void CSysConfig::setKeyData(const CString& section, const CString& key, const CString& value) {

    CString iniPath = getAppPath() + _T("config.ini");
    CIniWriter  iniWriter(iniPath);
    iniWriter.WriteString(section, key, value);
    return;
}


bool  CSysConfig::IsPrivateChatOpen() {
    return  m_pConfig.privateOpen == 1;
}

bool  CSysConfig::IsGroupChatOpen() {
    return  m_pConfig.groupOpen == 1;
}

bool  CSysConfig::IsCatKeyOpen() {
    return  m_pConfig.catKeyOpen == 1;
}
