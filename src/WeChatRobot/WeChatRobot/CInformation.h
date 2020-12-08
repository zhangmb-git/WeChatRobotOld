#pragma once

//个人信息的结构体
struct Information
{
	wchar_t wxid[40];		//微信ID
	wchar_t wxcount[40];	//微信账号
	wchar_t nickname[40];	//微信昵称
	wchar_t wxsex[4];		//性别
	wchar_t phone[30];		//手机号
	wchar_t device[15];		//登陆设备
	wchar_t nation[10];		//国籍
	wchar_t province[20];	//省份
	wchar_t city[20];		//城市
	wchar_t header[0x100];	//头像
};

// CInformation 对话框

class CInformation : public CDialogEx
{
	DECLARE_DYNAMIC(CInformation)

public:
	CInformation(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CInformation();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INFORMATION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	CString m_header;
	CString m_city;
	CString m_province;
	CString m_nation;
	CString m_device;
	CString m_phone;
	CString m_nickname;
	CString m_count;
	CString m_wxid;
	CString m_sex;
};
