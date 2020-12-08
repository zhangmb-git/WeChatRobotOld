#include "stdafx.h"
#include "base/Base64.h"
#include "HttpConn.h"
#include "json/json.h"
#include "common/public_define.h"
#include <afxwin.h>

#include "base/ZLogger.h"
#include "base/task_mgr.h"
#include "pangmao/GroupTask.h"
#include "CSysConfig.h"
#include "common/HttpClient.h"
#include "api/MsgManager.h"

#include <spdlog/formatter.h>


//#include <pb/protocol/IM.BaseDefine.pb.h>
//#define URL_MSG_ATALLMSG     "/msg/atAllMsg"
//#define URL_MSG_RECEIVEMSG   "/msg/receiveMsg"
#define URL_MSG_SENDMSG      "/msg/sendMsg"
#define URL_MSG_REGISTER_CALLBACK   "/msg/registerPushCb"

#define URL_GET_FRIENDLIST   "/friend/getFriendList"
#define URL_FRIEND_ADD       "/friend/add"
#define URL_FRIEND_DEL       "/friend/delete"

#define URL_GROUP_CHANGE_NAME  "/group/changeName"
#define URL_GROUP_GROUPLIST    "/group/getChatroomList"
#define URL_GROUP_MEMBER_LIST  "/group/memberlist"
#define URL_GROUP_MEMBER_ADD   "/group/addMember"
#define URL_GROUP_MEMBER_DEL   "/group/delMember"

#define URL_USER_BASEINFO      "/user/baseInfo"
#define URL_USER_LOGIN         "/user/loginInfo"


static HttpConnMap_t g_http_conn_map;

static rb_timer_item g_httpConn_timer;
// conn_handle 从0开始递增，可以防止因socket handle重用引起的一些冲突.
static uint32_t g_conn_handle_generator = 0;

typedef std::function<void(uint32_t, const string&, const string&)> HttpRequestCallback;

std::map<string, HttpRequestCallback> handleMap;

CHttpConn* FindHttpConnByHandle(uint32_t conn_handle) {
    CHttpConn* pConn = NULL;
    HttpConnMap_t::iterator it = g_http_conn_map.find(conn_handle);

    if (it != g_http_conn_map.end()) {
        pConn = it->second;
    }

    return pConn;
}

void httpconn_callback(void* callback_data, uint8_t msg, uint32_t handle, uint32_t uParam, void* pParam) {
    NOTUSED_ARG(uParam);
    NOTUSED_ARG(pParam);

    // convert void* to uint32_t, oops
    uint32_t conn_handle = *((uint32_t*)(&callback_data));
    CHttpConn* pConn = FindHttpConnByHandle(conn_handle);

    if (!pConn) {
        return;
    }

    switch (msg) {
    case NETLIB_MSG_READ:
        pConn->OnRead();
        break;

    case NETLIB_MSG_WRITE:
        pConn->OnWrite();
        break;

    case NETLIB_MSG_CLOSE:
        pConn->OnClose();
        break;

    default:
        LogError("!!!httpconn_callback error msg: {} ", msg);
        break;
    }
}

void http_conn_timer_callback(void* callback_data, uint8_t msg, uint32_t handle, void* pParam) {
    (void) callback_data;
    (void) msg;
    (void) handle;
    (void) pParam;

    CHttpConn* pConn = NULL;
    HttpConnMap_t::iterator it, it_old;
    uint64_t cur_time = get_tick_count();

    for (it = g_http_conn_map.begin(); it != g_http_conn_map.end();) {
        it_old = it;
        it++;

        pConn = it_old->second;
        pConn->OnTimer(cur_time);
    }
}

void init_http_conn() {
    rbtimer_init(&g_httpConn_timer, http_conn_timer_callback, NULL, 1000, 0, 0);
    netlib_register_timer(&g_httpConn_timer);

    // init handle map
    handleMap[URL_GET_FRIENDLIST] = std::move(onGetFriendList);
    handleMap[URL_FRIEND_ADD] = std::move(onFriendAdd);
    handleMap[URL_FRIEND_DEL] = std::move(onFriendDel);

    handleMap[URL_GROUP_CHANGE_NAME] = std::move(onGroupChangeName);
    handleMap[URL_GROUP_GROUPLIST] = std::move(onGroupGetList);

    handleMap[URL_GROUP_MEMBER_LIST] = std::move(onGroupMemberList);
    handleMap[URL_GROUP_MEMBER_ADD] = std::move(onGroupMemberAdd);
    handleMap[URL_GROUP_MEMBER_DEL] = std::move(onGroupMemberDel);

    handleMap[URL_MSG_SENDMSG] = std::move(onSendMsg);
    //handleMap[URL_MSG_ATALLMSG] = std::move(onSendAtAllMsg);  合并到sendMsg
    handleMap[URL_MSG_REGISTER_CALLBACK] = std::move(onRegisterReceiveMsgCallback);

    handleMap[URL_USER_BASEINFO] = std::move(onFriendGetAccountInfo);
    handleMap[URL_USER_LOGIN] = std::move(onFriendFreshQrCode);
}

//////////////////////////
CHttpConn::CHttpConn() {
    m_busy = false;
    m_sock_handle = NETLIB_INVALID_HANDLE;
    m_state = CONN_STATE_IDLE;

    m_last_send_tick = m_last_recv_tick = get_tick_count();
    m_conn_handle = ++g_conn_handle_generator;

    if (m_conn_handle == 0) {
        m_conn_handle = ++g_conn_handle_generator;
    }

    //log("CHttpConn, handle=%u\n", m_conn_handle);
}

CHttpConn::~CHttpConn() {
    //log("~CHttpConn, handle=%u\n", m_conn_handle);
}

int CHttpConn::Send(void* data, int len) {
    m_last_send_tick = get_tick_count();

    if (m_busy) {
        m_out_buf.Write(data, len);
        return len;
    }

    int ret = netlib_send(m_sock_handle, data, len);

    if (ret < 0)
        ret = 0;

    if (ret < len) {
        m_out_buf.Write((char*) data + ret, len - ret);
        m_busy = true;
        //log("not send all, remain=%d\n", m_out_buf.GetWriteOffset());

    } else {
        OnWriteComplete();
    }

    return len;
}

void CHttpConn::Close() {
    m_state = CONN_STATE_CLOSED;

    g_http_conn_map.erase(m_conn_handle);
    netlib_close(m_sock_handle);
}

void CHttpConn::OnConnect(net_handle_t handle) {
    printf("OnConnect, handle=%d\n", handle);
    m_sock_handle = handle;
    m_state = CONN_STATE_CONNECTED;
    g_http_conn_map.insert(make_pair(m_conn_handle, this));

    netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void*) httpconn_callback);
    netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, reinterpret_cast<void*>(m_conn_handle));
    netlib_option(handle, NETLIB_OPT_GET_REMOTE_IP, (void*) &m_peer_ip);
}

void CHttpConn::OnRead() {
    LogInfo("for http conn, remote ip:{}, remote port:{}", GetPeerIP(), m_peer_port);

    for (;;) {
        uint32_t free_buf_len = m_in_buf.GetAllocSize() - m_in_buf.GetWriteOffset();

        if (free_buf_len < READ_BUF_SIZE + 1)
            m_in_buf.Extend(READ_BUF_SIZE + 1);

        int ret = netlib_recv(m_sock_handle, m_in_buf.GetBuffer() + m_in_buf.GetWriteOffset(), READ_BUF_SIZE);

        if (ret <= 0)
            break;

        m_in_buf.IncWriteOffset(ret);

        m_last_recv_tick = get_tick_count();
    }

    // 每次请求对应一个HTTP连接，所以读完数据后，不用在同一个连接里面准备读取下个请求.
    char* in_buf = (char*) m_in_buf.GetBuffer();
    uint32_t buf_len = m_in_buf.GetWriteOffset();
    in_buf[buf_len] = '\0';

    // 如果buf_len 过长可能是受到攻击，则断开连接
    // 正常的url最大长度为2048，我们接受的所有数据长度不得大于1K.
    if (buf_len > 1024) {
        LogError("get too much data:{} ", buf_len);
        Close();
        return;
    }

    //log("OnRead, buf_len=%u, conn_handle=%u\n", buf_len, m_conn_handle); // for debug
    m_cHttpParser.ParseHttpContent(in_buf, buf_len);

    if (m_cHttpParser.IsReadAll()) {
        LogInfo("get original url:{}", m_cHttpParser.GetUrl());
        string url = _urlDecode(m_cHttpParser.GetUrl());

        if (url == "") {
            LogError("illegal request url format");
            Close();
            return;
        }

        std::string body = m_cHttpParser.GetBodyContent();
        LogInfo("parsed url:{},body:{}", url.c_str(), body);

        if (!_HandleRequest(url, body)) {
            LogError("url unknown, url={} ", url.c_str());
            Close();
        }

    } else {
        Close();
    }
}

void CHttpConn::OnWrite() {
    if (!m_busy)
        return;

    int ret = netlib_send(m_sock_handle, m_out_buf.GetBuffer(), m_out_buf.GetWriteOffset());

    if (ret < 0)
        ret = 0;

    int out_buf_size = (int) m_out_buf.GetWriteOffset();

    m_out_buf.Read(NULL, ret);

    if (ret < out_buf_size) {
        m_busy = true;
        LogError("not send all, remain={} ", m_out_buf.GetWriteOffset());

    } else {
        OnWriteComplete();
        m_busy = false;
    }
}

void CHttpConn::OnClose() {
    Close();
}

void CHttpConn::OnTimer(uint64_t curr_tick) {
    if (curr_tick > m_last_recv_tick + HTTP_CONN_TIMEOUT) {
        LogInfo("HttpConn timeout, handle={} ", m_conn_handle);
        Close();
    }
}


void CHttpConn::OnWriteComplete() {
//    log("write complete ");
    Close();
}

void CHttpConn::SendError(int code, string msg) {
    Json::Value root;
    root["resultCode"] = code;
    root["resultMsg"] = msg;
    std::string content = root.toStyledString();

    char* body = new char[HTTP_RESPONSE_HTML_MAX];
    snprintf(body, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_JSON, content.length(), content.c_str());
    Send((void*) body, strlen(body));
    delete[] body;
}

void CHttpConn::SendJson(string jsonContent) {
    LogInfo("SendJson res:{}", jsonContent);
    jsonContent = string_To_UTF8(jsonContent);

    char* body = new char[HTTP_RESPONSE_HTML_MAX];
    snprintf(body, HTTP_RESPONSE_HTML_MAX, HTTP_RESPONSE_JSON, jsonContent.length(), jsonContent.c_str());
    Send((void*) body, strlen(body));
    delete[] body;

    Close();
}

unsigned char CHttpConn::_fromHex(unsigned char x) {
    unsigned char y;

    if (x >= 'A' && x <= 'F') y = x - 'A' + 10;

    else if (x >= 'a' && x <= 'f') y = x - 'a' + 10;

    else if (x >= '0' && x <= '9') y = x - '0';

    //else assert(0);
    else y = 255;

    return y;
}


std::string CHttpConn::_urlDecode(const std::string& str) {
    std::string strTemp = "";
    size_t length = str.length();

    for (size_t i = 0; i < length; i++) {
        if (str[i] == '+') strTemp += ' ';

        else if (str[i] == '%') {
            //assert(i + 2 < length);
            if (i + 2 >= length) {
                return "";
            }

            unsigned char high = _fromHex((unsigned char) str[++i]);

            if (high == 255) {
                return "";
            }

            unsigned char low = _fromHex((unsigned char) str[++i]);

            if (low == 255) {
                return "";
            }

            strTemp += high * 16 + low;

        } else strTemp += str[i];
    }

    return strTemp;
}

bool CHttpConn::_HandleRequest(std::string url, std::string body) {
    auto callback = handleMap[url.c_str()];

    if (callback != nullptr) {
        LogInfo("handle request handle:{}, url:{}, body:{}", this->m_conn_handle, url, body);

        callback(this->m_conn_handle, url, body);
        return true;
    }

    return false;
}

void _sendJson(uint32_t handle, string jsonContent) {
    auto conn = FindHttpConnByHandle(handle);

    if (conn != nullptr) {
        conn->SendJson(jsonContent);
    }
}

//-- 用户管理
void onFriendGetAccountInfo(uint32_t handle, const std::string& url, const std::string& body) {

}
void onFriendFreshQrCode(uint32_t handle, const std::string& url, const std::string& body) {

}

//-- 好友管理
void onGetFriendList(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value  root;
    Json::Value data;

    for (auto& iter : g_WxData.mapFriendList) {
        size_t pos1 = iter.first.find("@chatroom");
        bool isChatroom = pos1 != string::npos;
        // size_t pos2 = iter.second.find("gh_");
        // bool isGh = pos2 != string::npos;

        if (!isChatroom) {
            Json::Value  value;
            value["wxID"] = iter.first;
            value["wxNickName"] = iter.second;
            data.append(value);

        }
    }

    root["friendList"] = data;
    root["errorMsg"] = "success";
    root["errorCode"] = 0;

    _sendJson(handle, root.toStyledString());
}

void onFriendAdd(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (root["friendlist"].isNull()) {
        value["errorMsg"] = "miss to,sessionType,msgType or msgContent field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    for (auto& iter : root["friendlist"]) {
        std::string strWxFriendID = iter.asString();
        CMsgManager::GetInstance()->SendFriendAddMsg(CString(strWxFriendID.c_str()), _T(""));
    }

    _sendJson(handle, value.toStyledString());
    return;
}

void onFriendDel(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (root["friendlist"].isNull()) {
        value["errorMsg"] = "miss to,sessionType,msgType or msgContent field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    for (auto& iter : root["friendlist"]) {
        std::string strWxFriendID = iter.asString();
        CMsgManager::GetInstance()->SendFriendDelMsg(CString(strWxFriendID.c_str()));
    }

    _sendJson(handle, value.toStyledString());
    return;
}

//-- 群组管理
void onGroupChangeName(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, root.toStyledString());
        return;
    }

    if (root["wxGroupId"].isNull() || root["newGroupName"].isNull()) {
        value["errorMsg"] = "miss to,sessionType,msgType or msgContent field";
        _sendJson(handle, root.toStyledString());
        return;
    }

    std::string  strGroupID = root["wxGroupId"].asString();
    std::string  strGroupName = root["newGroupName"].asString();

    if (!CMsgManager::GetInstance()->SendFixGroupNameMsg(CString(strGroupID.c_str()), CString(strGroupName.c_str()))) {
        value["errorMsg"] = "send fix group name msg error";
    }

    _sendJson(handle, value.toStyledString());
    return;
}

void onGroupGetList(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value  root;
    Json::Value data;

    for (auto& iter : g_WxData.mapFriendList) {
        size_t pos1 = iter.first.find("@chatroom");
        bool isChatroom = pos1 != string::npos;

        if (isChatroom) {
            Json::Value  value;
            value["wxID"] = iter.first;
            value["wxNickName"] = iter.second;
            data.append(value);
        }
    }

    root["groupList"] = data;
    root["errorMsg"] = "success";
    root["errorCode"] = 0;

    _sendJson(handle, root.toStyledString());
}

//-- 群成员管理
void onGroupMemberList(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (root["wxGroupId"].isNull()) {
        value["errorMsg"] = "miss to,sessionType,msgType or msgContent field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    std::string  strGroupID = root["wxGroupId"].asString();
    value["wxGroupId"] = strGroupID;

    if (g_WxData.mapGroupMemberStatus[strGroupID] == true) {

        Json::Value groupMember;

        for (auto& iter : g_WxData.mapGroupMemberInfo[strGroupID]) {
            groupMember[iter.first] = iter.second;
        }

        value["members"] = groupMember;
        _sendJson(handle, value.toStyledString());
        return;
    }

    //主线程回调
    auto httpCallback = [handle, strGroupID] {

        Json::Value value;
        value["errorMsg"] = "success";
        value["errorCode"] = 0;
        value["wxGroupId"] = strGroupID;

        if (g_WxData.mapGroupMemberStatus[strGroupID] == true) {

            Json::Value groupMember;

            for (auto& iter : g_WxData.mapGroupMemberInfo[strGroupID]) {
                groupMember[iter.first] = iter.second;
            }

            value["members"] = groupMember;


        } else {
            value["errorMsg"] = "get group member list error";   //
            value["errorCode"] = 1;                              //
        }

        _sendJson(handle, value.toStyledString());
        return;
    };

    //添加任务
    CGroupTask* pGroupTask = new CGroupTask;
    pGroupTask->SetHttpCallBack(httpCallback);
    CGroupTaskMgr::getInstance()->AddTask(strGroupID, pGroupTask);
    pGroupTask->SendGroupMemberRequest(CString(strGroupID.c_str()));
    return;
}

void onGroupMemberAdd(uint32_t handle, const std::string& url, const std::string& body) {

    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    //
    if (root["memberInfo"].isNull()) {
        value["errorMsg"] = "missmemberInfo  field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    for (auto& itor : root["memberInfo"]) {
        Json::Value&  jsonMemberInfo = itor;
        std::string  strGroupID = jsonMemberInfo["wxGroupId"].asString();

        //发送添加成员信息
        for (auto& iter : jsonMemberInfo["members"]) {
            std::string  strMemberID = iter.asString();
            CMsgManager::GetInstance()->SendGroupMemberOperMsg(E_Msg_AddGroupMember, CString(strGroupID.c_str()), CString(strMemberID.c_str()));
        }
    }

    _sendJson(handle, value.toStyledString());
    return;
}

void onGroupMemberDel(uint32_t handle, const std::string& url, const std::string& body) {

    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    //
    if (root["memberInfo"].isNull()) {
        value["errorMsg"] = "miss memberInfo  field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    for (auto& itor : root["memberInfo"]) {
        Json::Value&  jsonMemberInfo = itor;
        std::string  strGroupID = jsonMemberInfo["wxGroupId"].asString();

        //发送添加成员信息
        for (auto& iter : jsonMemberInfo["members"]) {
            std::string  strMemberID = iter.asString();
            CMsgManager::GetInstance()->SendGroupMemberOperMsg(E_Msg_DelRoomMember, CString(strGroupID.c_str()), CString(strMemberID.c_str()));
        }
    }

    _sendJson(handle, value.toStyledString());
    return;
}


//-- 消息管理
void onSendMsg(uint32_t handle, const std::string& url, const std::string& body) {
    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (root["to"].isNull() || root["sessionType"].isNull() || root["msgType"].isNull() || root["msgContent"].isNull()) {
        value["errorMsg"] = "miss to,sessionType,msgType or msgContent field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    std::string to;
    std::string session_type;
    std::string msg_type;

    if (root["to"].isString()) {
        to = root["to"].asString();
    }

    if (root["sessionType"].isString()) {
        session_type = root["sessionType"].asString();
    }

    if (root["msgType"].isString()) {
        msg_type = root["msgType"].asString();
    }

    if (to.empty() || session_type.empty() || msg_type.empty()) {
        value["errorMsg"] = "invalid to,sessionType or msgType field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (strcmp("single", session_type.c_str()) != 0 && strcmp("group", session_type.c_str()) != 0) {
        value["errorMsg"] = "invalid session_type";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (strcmp("group", session_type.c_str()) == 0) {
        size_t pos = to.find("chatroom");

        if (pos == std::string::npos) {
            value["errorMsg"] = "invalid group id";
            _sendJson(handle, value.toStyledString());
            return;
        }
    }

    int wx_msg_type = -1;
    CString cs_msg;
    CString cs_to = stringToCString(to, CP_UTF8);

    if (strcmp("text", msg_type.c_str()) == 0) {
        wx_msg_type = msg_type_text;

        if (root["msgContent"].isString()) {
            cs_msg = stringToCString(root["msgContent"].asString(), CP_UTF8);
        }

    } else if (strcmp("image", msg_type.c_str()) == 0) {
        wx_msg_type = msg_type_pic;

        if (root["msgContent"]["url"].isString()) {
            cs_msg = stringToCString(root["msgContent"]["url"].asString(), CP_UTF8);
        }

    } else if (strcmp("video", msg_type.c_str()) == 0) {
        wx_msg_type = msg_type_video;

        if (root["msgContent"]["url"].isString()) {
            cs_msg = stringToCString(root["msgContent"]["url"].asString(), CP_UTF8);
        }

    } else if (strcmp("file", msg_type.c_str()) == 0) {
        wx_msg_type = msg_type_sharelink_file;

        if (root["msgContent"]["url"].isString()) {
            cs_msg = stringToCString(root["msgContent"]["url"].asString(), CP_UTF8);
        }

    } else if (strcmp("card", msg_type.c_str()) == 0) { // 名片
        if (root["msgContent"]["wxId"].isString() && root["msgContent"]["wxNickName"].isString()) {
            std::string id = root["msgContent"]["wxId"].asString();
            CString  wx_id = stringToCString(root["msgContent"]["wxId"].asString(), CP_UTF8);
            CString  wx_nick_name = stringToCString(root["msgContent"]["wxNickName"].asString(), CP_UTF8);

            if (wx_id.IsEmpty() || wx_nick_name.IsEmpty()) {
                LogWarn("invalid wxId or wxNickName");
                return;
            }

            size_t pos = id.find("@chatroom");

            if (pos != string::npos) {
                LogWarn("invalid wxId,not support @chatroom");
                return;
            }

            CMsgManager::GetInstance()->SendCard(cs_to, wx_id, wx_nick_name);
            _sendJson(handle, value.toStyledString());
        }

        return;

    } else if (strcmp("atall", msg_type.c_str()) == 0) {
        wx_msg_type = msg_type_atallmsg;

        if (root["msgContent"].isString()) {
            cs_msg = stringToCString(root["msgContent"].asString(), CP_UTF8);
        }

    }

    if (wx_msg_type == -1) {
        value["errorMsg"] = "invalid msgType";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (cs_msg.IsEmpty()) {
        value["errorMsg"] = "invalid msgContent";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (wx_msg_type == msg_type_text) {
        CMsgManager::GetInstance()->SendMsg(wx_msg_type, cs_to, cs_msg);

    } else {
        _onSendFileMsg(wx_msg_type, to, cStringToString(cs_msg, CP_UTF8));
    }

    _sendJson(handle, value.toStyledString());
}

void _onSendFileMsg(int msg_type, const string wxid, const string url) {
    auto callback = [wxid, msg_type, url](void* data) {
        std::string path;
        int type2 = msg_type;

        if (msg_type == msg_type_pic) {
            path = CMsgManager::GetInstance()->DownloadImageFromUrl(url).c_str();

        } else if (msg_type == msg_type_sharelink_file || msg_type == msg_type_video) {
            path = CMsgManager::GetInstance()->DownloadFileFromUrl(msg_type_sharelink_file, url).c_str();
            type2 = (int)msg_type_sharelink_file;;
        }

        if (path.empty()) {
            LogWarn("download file error:{}", url);
            return;
        }

        LogInfo("send file,local_path:{},msg_type:{}", path, msg_type);
        CString id = stringToCString(wxid, CP_UTF8);
        CString msg_content = stringToCString(string_To_UTF8(path), CP_UTF8);
        CMsgManager::GetInstance()->SendMsg(type2, id, msg_content);
    };

    CTask* task = new CTask;
    task->SetTaskParam(nullptr);
    task->SetCallBack(callback);
    CTaskMgr::getInstance()->AddTask(wxid, task);
}

void onSendAtAllMsg(uint32_t handle, const std::string& url, const std::string& body) {
}

void onRegisterReceiveMsgCallback(uint32_t handle, const std::string& url, const std::string& body) {
    LogInfo("onRegisterReceiveMsgCallback...");

    Json::Value root;
    Json::Reader* reader = new Json::Reader(Json::Features::strictMode());
    bool success = reader->parse(body, root);
    delete reader;

    Json::Value value;
    value["errorMsg"] = "success";
    value["errorCode"] = 0;

    if (!success) {
        value["errorMsg"] = "invalid json data";
        _sendJson(handle, value.toStyledString());
        return;
    }

    if (root["wxId"].isNull() || root["url"].isNull()) {
        value["errorMsg"] = "miss wxId or url field";
        _sendJson(handle, value.toStyledString());
        return;
    }

    // res
    std::string  strWxID = root["wxId"].asString();
    std::string  strUrl = root["url"].asString();

    module::getSysConfigModule()->setKeyData(_T("PUSH"), _T("PushUrl"), CString(strUrl.c_str()));
    module::getSysConfigModule()->getSysConfig()->pushUrl = strUrl;
    _sendJson(handle, value.toStyledString());

#if 0 // 后续要实现ping机制，不能直接假调用。
    std::string  strResp;
    CHttpClient  client;
    CURLcode code = client.PostJson(strUrl, "{}", strResp);

    if (code != CURLE_OK) {
        LogError("register receive msg callback url err,url:{},respcode:{}", strUrl, code);
        value["errorMsg"] = fmt::format("url:{} not available,rescode:{}", strUrl, code);
        _sendJson(handle, value.toStyledString());
    }

#endif
}