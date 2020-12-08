#pragma once
#include <memory>
#include <list>
#include <deque>
#include <thread>
#include <map>
#include <unordered_map>
#include <mutex>
#include "thread_pool.h"


class CTaskMgr
{
public:
	static CTaskMgr * getInstance();
	~CTaskMgr();

	void  AddTask(int taskID,tagTask* task);
	void  AddTask(const std::string& strTask, tagTask* task);

	void  Loop();
	void  Stop();
private:
	static  CTaskMgr*  s_pTaskMgr ;
	CTaskMgr();
	CThreadPool  m_threadpool;
	
	
};

