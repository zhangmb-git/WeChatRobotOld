#include "../stdafx.h"
#include <string>
#include "HttpClient.h"
#include "json/json.h"
#include "json/reader.h"
#include "../base/ZLogger.h"

using namespace std;

////这个函数是为了符合CURLOPT_WRITEFUNCTION而构造的
//完成数据保存功能
struct FtpFile {
    const char* filename;
    FILE* stream;
    const char* file_path;
};

size_t write_data_string(void* ptr, size_t size, size_t nmemb, void* userp) {
    size_t len = size * nmemb;
    string* response = (string*) userp;

    response->append((char*) ptr, len);

    return len;
}

CHttpClient::CHttpClient(void) {

}

CHttpClient::~CHttpClient(void) {

}

static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid) {
    string* str = dynamic_cast<string*>((string*) lpVoid);

    if (NULL == str || NULL == buffer) {
        return -1;
    }

    char* pData = (char*) buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}

CURLcode CHttpClient::Post(const string& strUrl, const string& strPost, string& strResponse) {
    CURLcode res;
    CURL* curl = curl_easy_init();

    if (NULL == curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

CURLcode CHttpClient::PostJson(const string& strUrl, const string& strPost, string& strResponse) {
    CURLcode res;
    CURL* curl = curl_easy_init();

    if (NULL == curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strPost.length());

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);  //设置超时为30
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // free headers
    curl_slist_free_all(headers);

    return res;
}

CURLcode CHttpClient::Get(const string& strUrl, string& strResponse) {
    CURLcode res;
    CURL* curl = curl_easy_init();

    if (NULL == curl) {
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &strResponse);
    /**
     * 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
     * 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
     */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

string CHttpClient::UploadByteFile(const string& strUrl, void* pData, int nSize) {
    if (strUrl.empty())
        return "";

    CURL* curl = curl_easy_init();

    if (!curl)
        return "";

    struct curl_slist* headerlist = NULL;
    headerlist = curl_slist_append(headerlist,
                                   "Content-Type: multipart/form-data; boundary=WebKitFormBoundary8riBH6S4ZsoT69so");
    // what URL that receives this POST
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
//    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    //enable verbose for easier tracing
    string body = "--WebKitFormBoundary8riBH6S4ZsoT69so\r\nContent-Disposition: form-data; name=\"file\"; filename=\"1.audio\"\r\nContent-Type:image/jpg\r\n\r\n";
    body.append((char*) pData, nSize);     // image buffer
    string str = "\r\n--WebKitFormBoundary8riBH6S4ZsoT69so--\r\n\r\n";
    body.append(str.c_str(), str.size());
    // post binary data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    // set the size of the postfields data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    string strResp;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResp);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    // Perform the request, res will get the return code
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (CURLE_OK != res) {
        LogError("curl_easy_perform failed, res={}", res);
        return "";
    }

    // upload 返回的json格式不一样，要特殊处理.
    Json::Reader  reader;
    Json::Value   value;


    if (!reader.parse(strResp, value)) {
        LogError("json parse failed: {}", strResp.c_str());
        return "";
    }

    if (value["error_code"].isNull()) {
        LogError("no code in response {}", strResp.c_str());
        return "";
    }

    uint32_t nRet = value["error_code"].asUInt();

    if (nRet != 0) {
        LogError("upload faile:{}", nRet);
        return "";
    }

    return value["url"].asString();
}

static size_t my_fwrite(void* buffer, size_t size, size_t nmemb, void* stream) {
    struct FtpFile* out = (struct FtpFile*)stream;

    if (out && !out->stream) {
        /* open file for writing */
        out->stream = fopen(out->filename, "wb");

        if (!out->stream)
            return -1; /* failure, can‘t open file to write */
    }

    return fwrite(buffer, size, nmemb, out->stream);
}

bool CHttpClient::DownloadByteFile(const string& url, const string& save_file_path) {
    if (url.empty())
        return false;

    struct FtpFile ftpfile = { 0 };
    CURLcode res = CURLE_OK;
    CURL* curl_handle = nullptr;

    ftpfile.file_path = save_file_path.c_str();
    ftpfile.filename = save_file_path.c_str();

    // init the curl session
    curl_handle = curl_easy_init();

    if (!curl_handle) {
        return false;
    }

    //specify URL to get
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    // send all data to this function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, my_fwrite);
    // we pass our 'chunk' struct to the callback function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&ftpfile);

    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);

    // some servers don't like requests that are made without a user-agent
    // field, so we provide one
    //curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // https, skip the verification of the server's certificate.
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, false);

    // 设置连接超时,单位:毫秒
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, 1 * 1000);
    // time out
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 20 * 1000);
    // set time out
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 3);
    // 执行数据请求
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // 释放资源
    if (ftpfile.stream)
        fclose(ftpfile.stream); /* close the local file */

    curl_easy_cleanup(curl_handle);

    return res == CURLE_OK;
}