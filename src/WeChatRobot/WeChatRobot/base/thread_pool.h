#pragma once
#include <memory>
#include <list>
#include <deque>
#include <thread>
#include <map>
#include <unordered_map>
#include "common_thread.hpp"


class CThreadPool
{
public:
	CThreadPool();
	~CThreadPool();

	void init(int num);
	void start();
	void stop();

	void addTask(int64_t taskID,tagTask* pTask);

private:
	std::vector<CQueueService*> m_threadArr;
	int  m_num;
	

};

