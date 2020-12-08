#pragma once

#include <vector>
#include <atlstr.h>

class CIniReader;
class CIniWriter;
class CSysConfig {
  public:
    CSysConfig();
    ~CSysConfig() = default;


    void setKeyData(const CString& section, const CString& key, const CString& value);
    std::string getImgPath();

  private:
    void _loadData();
    std::string  m_imgPath;

};

namespace module {
    CSysConfig* getSysConfigModule();
}

