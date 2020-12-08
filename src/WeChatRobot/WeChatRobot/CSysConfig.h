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
    int          chatMode;  //�ʴ�ģʽ0normal ����ģʽ  1race ����ģʽ
    int          pushFlag;  //��Ϣ���ͱ�ʶ
    int          transferFlag; //��Ϣת�����ܱ�ʶ
    int          imageFlag;  //ͼƬת����־
    int			 transferInterval; // ��Ѷת�������Ĭ��5����

    // ����
    int trafficLimit; // ��
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

