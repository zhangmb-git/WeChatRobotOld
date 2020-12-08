#include "thread_pool.h"


CThreadPool::CThreadPool()
{
}

CThreadPool::~CThreadPool()
{
	for (int i = 0; i < m_num; i++)
	{
		m_threadArr[i]->stop();
		if (m_threadArr[i])
		{
			delete  m_threadArr[i];
			m_threadArr[i] = nullptr;
		}
	}
}

void CThreadPool::init(int num)
{
	m_num = num;
	for (int i = 0; i < m_num; i++)
	{
		CQueueService * pThread= new CQueueService();
		m_threadArr.push_back(pThread);
	}
}

void CThreadPool::start()
{
	for (int i = 0; i < m_num; i++)
	{
		m_threadArr[i]->start();
	}
}

void CThreadPool::stop()
{
	for (int i = 0; i < m_num; i++)
	{
		m_threadArr[i]->stop();
	}
}

void CThreadPool::addTask(int64_t taskID, tagTask* pTask)
{
	int index = taskID % m_threadArr.size();
	m_threadArr[index]->addTask(pTask);
}