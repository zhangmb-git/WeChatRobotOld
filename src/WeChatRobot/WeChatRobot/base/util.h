#pragma once

#include <string>
#include <tchar.h>
//#include <atlstr.h>
//#pragma comment(lib,"atls.lib")
#include <atlstr.h>
#include <atlconv.h>

#ifndef _WIN32
#include <strings.h>
#endif

//#include <sys/stat.h>
#include <assert.h>
#include <vector>


#ifdef _WIN32
#define	snprintf	sprintf_s
#else
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#endif


#include <atlstr.h>

#define LOG_MODULE_IM         "IM"
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE	// remove warning C4996, 
#endif // !1
#define NOTUSED_ARG(v) ((void)v)		// used this to remove warning C4100, unreferenced parameter

/// yunfan modify end
//class CRefObject
//{
//public:
//	CRefObject();
//	virtual ~CRefObject();
//
//	void SetLock(CLock* lock) { m_lock = lock; }
//	void AddRef();
//	void ReleaseRef();
//private:
//	int				m_refCount;
//	CLock*	m_lock;
//};

uint64_t get_tick_count();
void util_sleep(uint32_t millisecond);


class CStrExplode {
  public:
    CStrExplode(char* str, char seperator);
    virtual ~CStrExplode();

    uint32_t GetItemCnt() {
        return m_item_cnt;
    }
    char* GetItem(uint32_t idx) {
        return m_item_list[idx];
    }
  private:
    uint32_t	m_item_cnt;
    char** 		m_item_list;
};

char* replaceStr(char* pSrc, char oldChar, char newChar);
std::string int2string(uint64_t user_id);
uint32_t string2int(const std::string& value);
uint64_t string2long(const std::string& value);
void replace_mark(std::string& str, std::string& new_value, uint32_t& begin_pos);
void replace_mark(std::string& str, uint32_t new_value, uint32_t& begin_pos);
void replace_all(std::string& str, const std::string& old_value, const std::string& new_value);
/**
 * 分隔字符串.
 string s = "a,b*c*d,e";
    vector<string> v;
    split_string(s, v, "*"); //a,b c d,e
    for (vector<string>::size_type i = 0; i != v.size(); ++i)
        cout << v[i] << " ";
    cout << endl;
 * @param source:源字符串
 * @param outStrings:输出
 * @param split:分隔符
 */
void split_string(const std::string& source, std::vector<std::string>& outStrings, const std::string& split);

// 去除空字符串
std::string& ltrim(std::string& str);
std::string& rtrim(std::string& str);
std::string trim(const std::string& str);

void writePid();
inline unsigned char toHex(const unsigned char& x);
inline unsigned char fromHex(const unsigned char& x);
std::string URLEncode(const std::string& sIn);
std::string URLDecode(const std::string& sIn);


int64_t get_file_size(const char* path);
const char*  memfind(const char* src_str, size_t src_len, const char* sub_str, size_t sub_len, bool flag = true);

//
std::string string_To_UTF8(const std::string& str);
std::string UTF8_To_string(const std::string& str);

std::string Wchar_tToString(wchar_t* wchar);
wchar_t* StringToWchar_t(const std::string& str);
void Log(const std::string& type, const std::string& wxid, const std::string& source, const std::string& msgSender, const std::string& content);

//std::string CString2String(const CString& src);
std::string cStringToString(const CString& src, UINT codepage = CP_UTF8);

CString stringToCString(const std::string& src, UINT codepage);
CString getAppPath();

std::wstring string2wstring(std::string str);
std::string wstring2string(std::wstring wstr);

//
void StringToArr(const std::string& src, std::vector<std::string>& vecDst, char sep = '|');

bool IsStrInVec(const std::string& target, const std::vector<std::string>& vec );
