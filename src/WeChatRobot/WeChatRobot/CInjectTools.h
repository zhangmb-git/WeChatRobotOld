#pragma once

DWORD ProcessNameFindPID(const char* ProcessName);	//ͨ����������ȡ����ID

BOOL InjectDll(HANDLE& wxPid, bool& is_repeat); //ע��dll
void UnloadDll(); //ж��DLL
BOOL CheckIsInject(DWORD dwProcessid);	//���DLL�Ƿ��Ѿ�ע��
