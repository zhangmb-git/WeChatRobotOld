#include "util.h"
#include <sstream>
#include <algorithm>
#include <functional>
#include <Shlwapi.h>
#include <string.h>
#include <time.h>




using namespace std;


//CSLog g_imlog = CSLog(LOG_MODULE_IM);
//
//CRefObject::CRefObject() {
//    m_lock = NULL;
//    m_refCount = 1;
//}
//
//CRefObject::~CRefObject() {
//
//}
//
//void CRefObject::AddRef() {
//    if (m_lock) {
//        m_lock->lock();
//        m_refCount++;
//        m_lock->unlock();
//    } else {
//        m_refCount++;
//    }
//}
//
//void CRefObject::ReleaseRef() {
//    if (m_lock) {
//        m_lock->lock();
//        m_refCount--;
//        if (m_refCount == 0) {
//            delete this;
//            return;
//        }
//        m_lock->unlock();
//    } else {
//        m_refCount--;
//        if (m_refCount == 0)
//            delete this;
//    }
//}

uint64_t get_tick_count() {
#ifdef _WIN32
    LARGE_INTEGER liCounter;
    LARGE_INTEGER liCurrent;

    if (!QueryPerformanceFrequency(&liCounter))
        return GetTickCount();

    QueryPerformanceCounter(&liCurrent);
    return (uint64_t)(liCurrent.QuadPart * 1000 / liCounter.QuadPart);
#else
    struct timeval tval;
    uint64_t ret_tick;

    gettimeofday(&tval, NULL);

    ret_tick = tval.tv_sec * 1000L + tval.tv_usec / 1000L;
    return ret_tick;
#endif
}

void util_sleep(uint32_t millisecond) {
#ifdef _WIN32
    Sleep(millisecond);
#else
    usleep(millisecond * 1000);
#endif
}

CStrExplode::CStrExplode(char* str, char seperator) {
    m_item_cnt = 1;
    char* pos = str;

    while (*pos) {
        if (*pos == seperator) {
            m_item_cnt++;
        }

        pos++;
    }

    m_item_list = new char* [m_item_cnt];

    int idx = 0;
    char* start = pos = str;

    while (*pos) {
        if (pos != start && *pos == seperator) {
            uint32_t len = pos - start;
            m_item_list[idx] = new char[len + 1];
            strncpy(m_item_list[idx], start, len);
            m_item_list[idx][len] = '\0';
            idx++;

            start = pos + 1;
        }

        pos++;
    }

    uint32_t len = pos - start;

    if (len != 0) {
        m_item_list[idx] = new char[len + 1];
        strncpy(m_item_list[idx], start, len);
        m_item_list[idx][len] = '\0';
    }
}

CStrExplode::~CStrExplode() {
    for (uint32_t i = 0; i < m_item_cnt; i++) {
        delete[] m_item_list[i];
    }

    delete[] m_item_list;
}

char* replaceStr(char* pSrc, char oldChar, char newChar) {
    if (NULL == pSrc) {
        return NULL;
    }

    char* pHead = pSrc;

    while (*pHead != '\0') {
        if (*pHead == oldChar) {
            *pHead = newChar;
        }

        ++pHead;
    }

    return pSrc;
}

string int2string(uint64_t user_id) {
    stringstream ss;
    ss << user_id;
    return ss.str();
}

uint32_t string2int(const string& value) {
    return (uint32_t) strtol(value.c_str(), NULL, 10);
}

uint64_t string2long(const string& value) {
    return (uint64_t) strtoul(value.c_str(), NULL, 10);
}

// 由于被替换的内容可能包含?号，所以需要更新开始搜寻的位置信息来避免替换刚刚插入的?号
void replace_mark(string& str, string& new_value, uint32_t& begin_pos) {
    string::size_type pos = str.find('?', begin_pos);

    if (pos == string::npos) {
        return;
    }

    string prime_new_value = "'" + new_value + "'";
    str.replace(pos, 1, prime_new_value);

    begin_pos = pos + prime_new_value.size();
}

void replace_all(string& str, const string& old_value, const string& new_value) {
    while (true) {
        string::size_type pos(0);

        if ((pos = str.find(old_value)) != string::npos)
            str.replace(pos, old_value.length(), new_value);

        else break;
    }
}

void split_string(const std::string& source, std::vector<std::string>& outStrings, const std::string& split) {
    std::string::size_type pos1, pos2;
    pos2 = source.find(split);
    pos1 = 0;

    while (std::string::npos != pos2) {
        outStrings.push_back(source.substr(pos1, pos2 - pos1));

        pos1 = pos2 + split.size();
        pos2 = source.find(split, pos1);
    }

    if (pos1 != source.length())
        outStrings.push_back(source.substr(pos1));
}

void replace_mark(string& str, uint32_t new_value, uint32_t& begin_pos) {
    stringstream ss;
    ss << new_value;

    string str_value = ss.str();
    string::size_type pos = str.find('?', begin_pos);

    if (pos == string::npos) {
        return;
    }

    str.replace(pos, 1, str_value);
    begin_pos = pos + str_value.size();
}

// 去除空字符串
string& ltrim(string& str) {
    string::iterator p = std::find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace)));
    str.erase(str.begin(), p);
    return str;
}

string& rtrim(string& str) {
    string::reverse_iterator p = find_if(str.rbegin(), str.rend(), not1(ptr_fun<int, int>(isspace)));
    str.erase(p.base(), str.end());
    return str;
}

string& trim(string& str) {
    ltrim(rtrim(str));
    return str;
}

void writePid() {
    uint32_t curPid;
#ifdef _WIN32
    curPid = (uint32_t) GetCurrentProcess();
#else
    curPid = (uint32_t) getpid();
#endif
    FILE* f = fopen("server.pid", "w");
    assert(f);
    char szPid[32];
    snprintf(szPid, sizeof(szPid), "%d", curPid);
    fwrite(szPid, strlen(szPid), 1, f);
    fclose(f);
}

inline unsigned char toHex(const unsigned char& x) {
    return x > 9 ? x - 10 + 'A' : x + '0';
}

inline unsigned char fromHex(const unsigned char& x) {
    return isdigit(x) ? x - '0' : x - 'A' + 10;
}

string URLEncode(const string& sIn) {
    string sOut;

    for (size_t ix = 0; ix < sIn.size(); ix++) {
        unsigned char buf[4];
        memset(buf, 0, 4);

        if (isalnum((unsigned char) sIn[ix])) {
            buf[0] = sIn[ix];
        }

        //else if ( isspace( (unsigned char)sIn[ix] ) ) //貌似把空格编码成%20或者+都可以
        //{
        //    buf[0] = '+';
        //}
        else {
            buf[0] = '%';
            buf[1] = toHex((unsigned char) sIn[ix] >> 4);
            buf[2] = toHex((unsigned char) sIn[ix] % 16);
        }

        sOut += (char*) buf;
    }

    return sOut;
}

string URLDecode(const string& sIn) {
    string sOut;

    for (size_t ix = 0; ix < sIn.size(); ix++) {
        unsigned char ch = 0;

        if (sIn[ix] == '%') {
            ch = (fromHex(sIn[ix + 1]) << 4);
            ch |= fromHex(sIn[ix + 2]);
            ix += 2;

        } else if (sIn[ix] == '+') {
            ch = ' ';

        } else {
            ch = sIn[ix];
        }

        sOut += (char) ch;
    }

    return sOut;
}


int64_t get_file_size(const char* path) {
    int64_t filesize = -1;
    struct stat statbuff;

    if (stat(path, &statbuff) < 0) {
        return filesize;

    } else {
        filesize = statbuff.st_size;
    }

    return filesize;
}

const char* memfind(const char* src_str, size_t src_len, const char* sub_str, size_t sub_len, bool flag) {
    if (NULL == src_str || NULL == sub_str || src_len <= 0) {
        return NULL;
    }

    if (src_len < sub_len) {
        return NULL;
    }

    const char* p;

    if (sub_len == 0)
        sub_len = strlen(sub_str);

    if (src_len == sub_len) {
        if (0 == (memcmp(src_str, sub_str, src_len))) {
            return src_str;

        } else {
            return NULL;
        }
    }

    if (flag) {
        for (int i = 0; i < src_len - sub_len; i++) {
            p = src_str + i;

            if (0 == memcmp(p, sub_str, sub_len))
                return p;
        }

    } else {
        for (int i = (src_len - sub_len); i >= 0; i--) {
            p = src_str + i;

            if (0 == memcmp(p, sub_str, sub_len))
                return p;

        }
    }

    return NULL;
}


//string转UTF8
std::string string_To_UTF8(const std::string& str) {
    int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

    wchar_t* pwBuf = new wchar_t[nwLen + 1]; //一定要加1，不然会出现尾巴
    ZeroMemory(pwBuf, nwLen * 2 + 2);

    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

    int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

    char* pBuf = new char[nLen + 1];
    ZeroMemory(pBuf, nLen + 1);

    ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

    std::string retStr(pBuf);

    delete[]pwBuf;
    delete[]pBuf;

    pwBuf = NULL;
    pBuf = NULL;

    return retStr;
}

std::string UTF8_To_string(const std::string& str) {
    int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* pwBuf = new wchar_t[nwLen + 1]; //一定要加1，不然会出现尾巴
    memset(pwBuf, 0, nwLen * 2 + 2);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);
    int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
    char* pBuf = new char[nLen + 1];
    memset(pBuf, 0, nLen + 1);
    WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
    std::string retStr = pBuf;
    delete[]pBuf;
    delete[]pwBuf;
    pBuf = NULL;
    pwBuf = NULL;
    return retStr;
}

//wchar_t转string
std::string Wchar_tToString(wchar_t* wchar) {
    std::string szDst;
    wchar_t* wText = wchar;
    DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用
    char* psText; // psText为char*的临时数组，作为赋值给std::string的中间变量
    psText = new char[dwNum];
    WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用
    szDst = psText;// std::string赋值
    delete[]psText;// psText的清除
    return szDst;
}


//string转wchar_t
wchar_t* StringToWchar_t(const std::string& str) {
    wchar_t* m_chatroommmsg = new wchar_t[str.size() * 2];   //
    memset(m_chatroommmsg, 0, str.size() * 2);
    setlocale(LC_ALL, "zh_CN.UTF-8");
    swprintf(m_chatroommmsg, str.size() * 2, L"%S", str.c_str());

    return m_chatroommmsg;
}


void Log(const std::string& type, const std::string& wxid, const std::string& source, const std::string& msgSender, const std::string& content) {
    setlocale(LC_ALL, "zh_CN.UTF-8");
    time_t t = time(0);
    char ch[64];
    strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S", localtime(&t)); //年-月-日 时-分-秒
    std::string times = ch;
    std::string log;

    if (strstr(msgSender.c_str(), "NULL") != NULL) {
        log = string_To_UTF8(
                  "************************ " + times + " ************************" +
                  "\n" + "消息类型:" + type +
                  "\n" + "消息来源:" + source +
                  "\n" + "微信ID:" + wxid +
                  "\n" + "消息内容:" + content +
                  "\n" + "-------------------------------- 分割线 --------------------------------\n\n\n"
              );

    } else {
        log = string_To_UTF8(
                  "************************ " + times + " ************************" +
                  "\n" + "消息类型:" + type +
                  "\n" + "消息来源:" + source +
                  "\n" + "群ID:" + wxid +
                  "\n" + "群发送者:" + msgSender +
                  "\n" + "消息内容:" + content +
                  "\n" + "-------------------------------- 分割线 --------------------------------\n\n\n"
              );
    }

    FILE* fp = fopen("log.txt", "ab+");
    fwrite(log.c_str(), log.length(), 1, fp);
    fclose(fp);
}

//std::string CString2String(const CString& src)
//{
//	std::string strRet;
//	USES_CONVERSION;
//	strRet = W2A(src);
//	return  strRet;
//}
//
//std::string cStringToString(const CString& src, UINT codepage /*= CP_UTF8*/) {
//    std::string dst;
//
//    if (src.IsEmpty()) {
//        dst.clear();
//        return "";
//    }
//
//    int length = ::WideCharToMultiByte(codepage, 0, src, src.GetLength(), NULL, 0, NULL, NULL);
//    dst.resize(length);
//    ::WideCharToMultiByte(codepage, 0, src, src.GetLength(), &dst[0], (int)dst.size(), NULL, NULL);
//
//    return dst;
//}

//CString stringToCString(const std::string& src, UINT codepage) {
//    CString dst;
//
//    if (src.empty()) {
//        return  dst;
//    }
//
//    int length = ::MultiByteToWideChar(codepage, 0, src.data(), (int)src.size(), NULL, 0);
//    WCHAR* pBuffer = dst.GetBufferSetLength(length);
//    ::MultiByteToWideChar(codepage, 0, src.data(), (int)src.size(), pBuffer, length);
//
//    return dst;
//}

std::wstring string2wstring(std::string str) {
    wstring result;
    //获取缓冲区大小，并申请空间，缓冲区大小按字符计算
    int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
    TCHAR* buffer = new TCHAR[len + 1];
    //多字节编码转换成宽字节编码
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
    buffer[len] = '\0';
    //添加字符串结尾
    //删除缓冲区并返回值
    result.append(buffer);
    delete[] buffer;
    return result;
}
//将wstring转换成string
std::string wstring2string(std::wstring wstr) {
    string result;
    //获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的
    int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
    char* buffer = new char[len + 1];
    //宽字节编码转换成多字节编码
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
    buffer[len] = '\0';
    //删除缓冲区并返回值
    result.append(buffer);
    delete[] buffer;
    return result;
}


void StringToArr(const std::string& src, std::vector<std::string>& vecDst,  char sep) {
    vecDst.clear();
    int  nCurPos = 0, nNextPos = 0;
    nNextPos = src.find_first_of(sep, nCurPos);

    while (nNextPos != std::string::npos) {
        std::string  tmpStr = src.substr(nCurPos, nNextPos - nCurPos);
        vecDst.push_back(tmpStr);
        nCurPos = nNextPos + 1;
        nNextPos = src.find_first_of(sep, nCurPos);
    }

    vecDst.push_back(src.substr(nCurPos));
    return;
}

//************************************************************
// 函数名称: CreateDir
// 函数说明: 创建目录
// 作    者: GuiShou
// 时    间: 2019/7/21
// 参    数: dir 目录
// 返 回 值: void
//************************************************************
void CreatePath(const char* dir) {

    CString folderPath(dir);

    if (!PathIsDirectory(folderPath)) { // 是否有重名文件夹
        ::CreateDirectory(folderPath, 0);
    }

    return;
}

