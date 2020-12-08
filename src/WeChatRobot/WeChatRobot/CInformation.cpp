#include "stdafx.h"
#include "Main.h"
#include "CInformation.h"
#include "afxdialogex.h"
#include "common/public_define.h"
#include "base/util.h"



// CInformation 对话框

IMPLEMENT_DYNAMIC(CInformation, CDialogEx)

CInformation::CInformation(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_INFORMATION, pParent)
    , m_header(_T(""))
    , m_city(_T(""))
    , m_province(_T(""))
    , m_nation(_T(""))
    , m_device(_T(""))
    , m_phone(_T(""))
    , m_nickname(_T(""))
    , m_count(_T(""))
    , m_wxid(_T(""))
    , m_sex(_T("")) {

}

CInformation::~CInformation() {
}

void CInformation::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_HEADER, m_header);
    DDX_Text(pDX, IDC_CITY, m_city);
    DDX_Text(pDX, IDC_PROVINCE, m_province);
    DDX_Text(pDX, IDC_NATION, m_nation);
    DDX_Text(pDX, IDC_DEVICE, m_device);
    DDX_Text(pDX, IDC_PHONE, m_phone);
    DDX_Text(pDX, IDC_NICKNAME, m_nickname);
    DDX_Text(pDX, IDC_ACCOUNT, m_count);
    DDX_Text(pDX, IDC_WXID, m_wxid);
    DDX_Text(pDX, IDC_SEX, m_sex);
}


BEGIN_MESSAGE_MAP(CInformation, CDialogEx)
    ON_WM_COPYDATA()
END_MESSAGE_MAP()


// CInformation 消息处理程序


BOOL CInformation::OnInitDialog() {
    CDialogEx::OnInitDialog();
    //查找窗口
    CWnd* pWnd = CWnd::FindWindow(NULL, L"WeChatHelper");
    COPYDATASTRUCT GetInformation;
    //组装数据
    GetInformation.dwData = WM_GetInformation;
    GetInformation.cbData = 0;
    GetInformation.lpData = NULL;
    //发送获取个人信息消息
    pWnd->SendMessage(WM_COPYDATA, NULL, (LPARAM)&GetInformation);

    return TRUE;
}


BOOL CInformation::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) {
    //显示好友列表
    if (pCopyDataStruct->dwData == WM_GetInformation) {
        //接收消息
        Information* info = new Information;
        info = (Information*)pCopyDataStruct->lpData;

        //显示到控件
        m_wxid = info->wxid;
        m_count = info->wxcount;
        m_nickname = info->nickname;
        m_phone = info->phone;
        m_device = info->device;
        m_nation = info->nation;
        m_city = info->city;
        m_province = info->province;
        m_header = info->header;
        m_sex = info->wxsex;

        if (!g_WxData.bGetInfoOK) {
            g_WxData.tPrivateInfo.wxid = Wchar_tToString(info->wxid);
            g_WxData.tPrivateInfo.wxcount = Wchar_tToString(info->wxcount);
            g_WxData.tPrivateInfo.nickname = Wchar_tToString(info->nickname);
            g_WxData.tPrivateInfo.phone = Wchar_tToString(info->phone);
            g_WxData.tPrivateInfo.device = Wchar_tToString(info->device);
            g_WxData.tPrivateInfo.nation = Wchar_tToString(info->nation);
            g_WxData.tPrivateInfo.city = Wchar_tToString(info->city);
            g_WxData.tPrivateInfo.province = Wchar_tToString(info->province);
            g_WxData.tPrivateInfo.header = Wchar_tToString(info->header);
            g_WxData.tPrivateInfo.wxsex = Wchar_tToString(info->wxsex);
            g_WxData.bGetInfoOK = true;  //获取个人信息ok
        }

        UpdateData(FALSE);
    }

    return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}
