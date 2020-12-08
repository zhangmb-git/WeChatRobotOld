#include "stdafx.h"

#include "MsgAutoForward.h"
#include "api/MsgReceive.h"
#include "api/MsgManager.h"
#include "CSysConfig.h"
#include "pangmao/MsgHelper.h"
#include "pangmao/MsgTrafficControl.h"


typedef struct MsgDataSend {
    int msgType;
    CString wxid;
    CString content;
} MsgDataSend;

CMsgAutoForward* CMsgAutoForward::instance_ = new CMsgAutoForward();

CMsgAutoForward::CMsgAutoForward(): is_forward_(false), is_forward_image_(false), is_init_(false), task_id_(0) {
    last_send_time_ = 0;
}

CMsgAutoForward* CMsgAutoForward::GetInstance() {
    return instance_;
}

void CMsgAutoForward::StartAutoForward() {
    if (is_init_) {
        return;
    }

    is_init_ = true;

    auto  pSysConfig = module::getSysConfigModule()->getSysConfig();

    if (pSysConfig == nullptr) {
        LogError("sysconfig nullptr");
        return;
    }

    if (!pSysConfig->transferFlag) {
        LogDebug("disable forward feature!");
        return;
    }

    is_forward_image_ = pSysConfig->imageFlag;
    forward_interval_ = pSysConfig->transferInterval;

    LogInfo("资讯转发功能已启动，转发频率：{} 秒钟1条", forward_interval_);

    // register
    auto recv_callback = std::bind(&CMsgAutoForward::onReceiveMsg, this, placeholders::_1);
    CMsgReceive::GetInstance()->Register(recv_callback, "CMsgAutoForward");

    // load listen
    for (auto item : module::getSysConfigModule()->getSysConfig()->vecFromGroup) {
        set_listen_.insert(item);
    }

    for (auto item : module::getSysConfigModule()->getSysConfig()->vecToGroup) {
        set_to_group_.insert(item);
    }

    if (set_listen_.size() == 0 || set_to_group_.size() == 0) {
        LogWarn("invalid vecToGroup or vecFromGroup");
        return;
    }

    // init thread pool
    thread_pool_ = make_unique<CThreadPool>();
    thread_pool_->init(4);
    thread_pool_->start();
}

void CMsgAutoForward::onReceiveMsg(const Message& msg) {
    LogDebug("from:{}", wstring2string(msg.wxid));

    auto it = set_listen_.find(wstring2string(msg.wxid));

    // not find, ignore
    if (it == set_listen_.end()) {
        LogDebug("not find from forward group");
        return;
    }

    switch (msg.msgType) {
    case msg_type_pic:
        onForwardImage(msg);
        break;

    case msg_type_text:
        onForwardText(msg);
        break;

    default:
        break;
    }
}

void CMsgAutoForward::onForwardText(const Message& msg) {
    CString  szContent = msg.content;
    CString  szNickName;

    if (!g_WxData.bGetInfoOK || module::getSysConfigModule()->IsCatKeyOpen()) {
        szNickName = stringToCString(module::getSysConfigModule()->getSysConfig()->catKey, CP_UTF8);

    } else {
        szNickName = g_WxData.tPrivateInfo.nickname.c_str();
    }

    szContent.Replace(_T("@" + szNickName), _T(""));
    szContent.TrimLeft();
    szContent.TrimRight();

    if (szContent.GetLength() > 400) {
        LogWarn("transfer msg  is too  long!");
        CMsgManager::GetInstance()->SendMsg(msg_type_text, msg.wxid, _T("转发资讯超长"));
        return ;
    }

    if (szContent.Find(_T("【")) == -1 && szContent.Find(_T("】")) == -1) {
        LogWarn("szContent format is not  right ,content:{} ", wstring2string(szContent.GetString()));
        //CMsgManager::GetInstance()->SendMsg(msg_type_text, msg.wxid, _T("资讯格式不符合要求"));
        return  ;
    }

    onBoradcastAsync(msg.wxid, msg_type_text, szContent);
}

void CMsgAutoForward::onForwardImage(const Message& msg) {
    if (!is_forward_image_) {
        return;
    }

    CString  szContent = msg.content;
    onBoradcastAsync(msg.wxid, msg_type_pic, szContent);
}

void CMsgAutoForward::onBoradcastAsync(const CString& fromId, const int& msg_type, const CString& content) {
    LogDebug("fromId:{},msgType:{}", cStringToString(fromId), msg_type);
    time_t cur = time(nullptr);

    int diff = ::abs(cur - last_send_time_);

    if (diff < forward_interval_) {
        int reset = forward_interval_ - diff;
        int min = reset / 60;
        int s = reset - (min * 60);
        char desc[32] = { 0 };

        min = min > 60 ? 59 : min;
        s = s > 60 ? 59 : s;

        if (min > 0) {
            sprintf(desc, "发送太快，请于%d分%d秒后重试", min, s);

        } else {
            sprintf(desc, "发送太快，请于%d秒后重试", s);
        }

        // 禁止刷屏
        bool start_limit = false;
        std::string limit_answer = "";

        if (CMsgTrafficControl::Grant(start_limit, limit_answer)) {
            CMsgManager::GetInstance()->SendMsg(msg_type_text, fromId, CString(desc));
        }

        return;
    }

    last_send_time_ = cur;

    auto callback = [](void* p) {
        MsgDataSend* msg = static_cast<MsgDataSend*>(p);

        CMsgManager::GetInstance()->SendMsg(msg->msgType, msg->wxid, msg->content);

        // 2秒发4个群。
        std::this_thread::sleep_for(2s);
    };

    for (auto& iter : set_to_group_) {
        MsgDataSend* msg = new MsgDataSend();
        msg->msgType = msg_type;
        msg->content = content;
        msg->wxid = CString(iter.c_str());

        CTask* t = new CTask();
        t->SetCallBack(callback);
        t->SetTaskParam((void*)msg);
        thread_pool_->addTask(++task_id_, t);
    }
}
