#pragma once
#include <afx.h>
#include <string>
#include <vector>



struct  tagConfig {
    std::string  wxGroup;
    std::string  catKey;
    std::string  catUrl;
    std::string  mqUrl;
    std::string  pushUrl;
    std::string  fileDir;
    std::string  imageDir;
    std::vector<std::string>  vecFromGroup;
    std::vector<std::string>  vecToGroup;

    int          catKeyOpen;
    int          privateOpen;
    int          groupOpen;
    int          chatMode;  //问答模式0normal 正常模式  1race 抢答模式
    int          pushFlag;  //消息推送标识
    int          transferFlag; //消息转发功能标识
    int          imageFlag;  //图片转发标志
    int			 transferInterval; // 资讯转发间隔，默认5分钟

    // 限流
    int trafficLimit; // 条
    int trafficInterval; // s


    std::string  serverIP;  //webserver
    int      port;

};


class CIniReader;
class CIniWriter;
class CSysConfig {
  public:
    CSysConfig();
    ~CSysConfig() = default;

    tagConfig* getSysConfig();
    void setKeyData(const CString& section, const CString& key, const CString& value);

    bool  IsPrivateChatOpen();
    bool  IsGroupChatOpen();
    bool  IsCatKeyOpen();

  private:

    void _loadData();
    tagConfig   m_pConfig;

};

namespace module {
    CSysConfig* getSysConfigModule();
}

