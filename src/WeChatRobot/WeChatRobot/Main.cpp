
// WeChatRobot.cpp: 定义应用程序的类行为。
//

#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "Main.h"
#include "WeChatRobotDlg.h"
#include "base/task_mgr.h"
#include "CrashRpt.h"
#include "common/HttpClient.h"
#include "base/ZLogger.h"
#include "pangmao/MsgTrafficControl.h"
#include "pangmao/MsgAutoForward.h"
#include "pangmao/MsgHelper.h"
#include "CSysConfig.h"
#include "api/http/HttpServer.h"
#include "api/http/HttpConn.h"
#include "CInjectTools.h"
#include "CMain.h"
#include "../../Network/tcp/TcpServer.h"
#include <asio.hpp>

#pragma comment(lib, "Network.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#pragma comment(lib,"CrashRpt1403d.lib")
#else
#pragma comment(lib,"CrashRpt1403.lib")
#endif

HANDLE wxPid = NULL;		//微信的PID

#ifdef _DEBUG
// added_by yignchun.xu 2020-06-19 同时打开控制台，便于调试
#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )
#endif

// CWeChatRobotApp

BEGIN_MESSAGE_MAP(CWeChatRobotApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CWeChatRobotApp 构造

CWeChatRobotApp::CWeChatRobotApp() {
    // 支持重新启动管理器
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: 在此处添加构造代码，
    // 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的 CWeChatRobotApp 对象

CWeChatRobotApp theApp;

// CWeChatRobotApp 初始化

void Inject(bool& needLogin) {
    // 防多开
    HANDLE hMutex = NULL;
    hMutex = CreateMutexA(NULL, FALSE, "GuiShou");

    if (hMutex) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            ExitProcess(-1);
        }
    }

    // DLL注入微信
    if (InjectDll(wxPid, needLogin) == FALSE) {
        ExitProcess(-1);
    }
}

BOOL CWeChatRobotApp::InitInstance() {
    // 如果一个运行在 Windows XP 上的应用程序清单指定要
    // 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
    //则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // 将它设置为包括所有要在应用程序中使用的
    // 公共控件类。
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();

    AfxEnableControlContainer();

    // 创建 shell 管理器，以防对话框包含
    // 任何 shell 树视图控件或 shell 列表视图控件。
    CShellManager* pShellManager = new CShellManager;

    // 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
    // CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
    //SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

    CR_INSTALL_INFO info;
    memset(&info, 0, sizeof(CR_INSTALL_INFO));
    info.cb = sizeof(CR_INSTALL_INFO);
    // Size of the structure
    info.pszAppName = _T("WeChatRobot");
    // App name
    info.pszAppVersion = _T("1.0.0");
    // App version
    info.pszEmailSubject = _T("CrashRpt Console Test 1.0.0 Error Report");
    // Email subject
    info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;
    info.dwFlags |= CR_INST_DONT_SEND_REPORT;
    // 不要发送邮件，保存到本地
    info.pszErrorReportSaveDir = TEXT("./crashRptData");
    int nInstResult = crInstall(&info);

    if (nInstResult != 0) {
        TCHAR buff[256];
        crGetLastErrorMsg(buff, 256); // Get last error
        _tprintf(_T("%s\n"), buff); // and output it to the screen
        std::string   strBuff = wstring2string(buff);
        LogError("install crashRpt error,{}", strBuff);
        return FALSE;
    }

    LogInfo("wechatrobot service start...");
    // added_by yingchun.xu 2020-06-28 流控
    CMsgTrafficControl::SetLimit(module::getSysConfigModule()->getSysConfig()->trafficInterval, module::getSysConfigModule()->getSysConfig()->trafficLimit);
    // added_by yingchun.xu 2020-08-06 资讯转发
    CMsgAutoForward::GetInstance()->StartAutoForward();
    // added_by yingchun.xu 2020-08-06 胖猫问答初始化
    CMsgHelper::GetInstance()->Init();

    //执行胖猫任务
    if (!CTaskMgr::getInstance()) {
        LogError("task mgr init error");
        return  FALSE;
    }

    //Tcp
    //asio::io_context  context;
    //TcpServer  tcpServer(context);
    TcpServer::getInstance()->start();
    //context.run();
    //TcpServer::getInstance()->start();
    Sleep(60000);
    return 1;
    //添加webServer
    CHttpServer  httpServer;
    init_http_conn();

    if (!httpServer.Start()) {
        LogError("http server start error ");
        return  FALSE;
    }

    // DLL注入微信+放多开本程序
    bool is_repeate = false;
    Inject(is_repeate);

#ifdef DEBUG

    // 如果已经注入，直接进入主窗口，否则进入登录界面
    if (is_repeate) {
        CMain mainWindow;
        m_pMainWnd = &mainWindow;
        mainWindow.DoModal();

    } else {
        CWeChatRobotDlg dlg;
        m_pMainWnd = &dlg;
        INT_PTR nResponse = dlg.DoModal();
    }

#else
    CWeChatRobotDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
#endif // DEBUG



    if (pShellManager != nullptr) {
        delete pShellManager;
    }

    httpServer.Stop();
    crUninstall();
    return FALSE;
}

