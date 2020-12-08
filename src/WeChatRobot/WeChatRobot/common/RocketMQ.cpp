#include "RocketMQ.h"
#include "MQMessageQueue.h"
#include "PullResult.h"

CRocketMQ::CRocketMQ(string strAdd, long long llset):m_consumer("DCS"),m_producer("DCS"),m_llOffSet(llset)
{
	vInit(strAdd, llset);
	m_bConsumerFlag = true;
	m_bProducerFlag = false;
}
CRocketMQ::CRocketMQ(string strAdd):m_consumer("DCS"),m_producer("DCS")
{
	vInit(strAdd);
	m_bProducerFlag = true;
	m_bConsumerFlag = false;
}
CRocketMQ::~CRocketMQ()
{
	if (m_bConsumerFlag)
	{
	// 停止消费者
	printf("[RocketMq]stop");
	m_consumer.shutdown();
	}
	if (m_bProducerFlag)
	{
	// 停止生产者
	printf("[RocketMq]stop");
	m_producer.shutdown();
	}
}
void CRocketMQ::vInit(string strAdd, long long llset) 
{
	// 设置MQ的NameServer地址
	printf("[RocketMq]Addr: %s\n", strAdd.c_str());
	m_consumer.setNamesrvAddr(strAdd);
	// 设置消费模式，CLUSTERING-集群模式，BROADCASTING-广播模式
	//printf("[RocketMq]Model: %s\n", getMessageModelString(CLUSTERING));
	//m_consumer.setMessageModel(CLUSTERING);
	// 非阻塞模式，拉取超时时间，默认10s
	//m_consumer.setConsumerPullTimeoutMillis(4000);
	// 长轮询模式，Consumer连接在Broker挂起最长时间，默认20s
	//m_consumer.setBrokerSuspendMaxTimeMillis(3000);
	// 长轮询模式，拉取超时时间，默认30s
	//m_consumer.setConsumerTimeoutMillisWhenSuspend(5000);
	// 启动消费者
	//printf("[RocketMq]start\n");
	m_consumer.start();
}
void CRocketMQ::vInit(string strAdd)
{
    // 设置MQ的NameServer地址
    printf("[RocketMq]Addr: %s\n", strAdd.c_str());
    m_producer.setNamesrvAddr(strAdd);
    // 启动消费者
    printf("[RocketMq]start\n");
    m_producer.start();
}

//获取消息，consumer消息
//返回值：当前set
long long CRocketMQ::lgetData(string strTopic, list<string>& lstrData)
{
	while(true)
	{
        // 获取指定topic的路由信息
        std::vector<MQMessageQueue> mqs;
		m_consumer.fetchSubscribeMessageQueues(strTopic,mqs);
		std::vector<MQMessageQueue>::iterator it = mqs.begin();
		bool nFirst = true;
		for (; it!=mqs.end(); it++)
		{
			MQMessageQueue& mq = *it;
			bool noNewMsg = false;
			while (!noNewMsg)
			{
				try
				{
					// 拉取消息
					//从指定set开始读取
					PullResult pullResult = m_consumer.pull(mq, "*", m_llOffSet, 32);
					if ((pullResult.pullStatus == FOUND) && (!pullResult.msgFoundList.empty()))
					{
						printf("[RocketMQ]获取到消息 %d 条,当前位置 %lld ,整体位置 %lld\n", pullResult.msgFoundList.size(), pullResult.nextBeginOffset, m_llOffSet);
						vector<MQMessageExt>::iterator it = pullResult.msgFoundList.begin();
						for (;it!=pullResult.msgFoundList.end();it++)
						{
								string strData = string(it->getBody());
								lstrData.push_back(strData);
						}
						//以防万一，此返回值存数大的
						m_llOffSet = (pullResult.nextBeginOffset > m_llOffSet ? pullResult.nextBeginOffset : m_llOffSet);
						//更新进度还用原来的
						m_consumer.updateConsumeOffset(mq, pullResult.nextBeginOffset);
					}
					else
					{
                         break;
					}
					//delete pullResult;
				}
				catch (MQException& e)
				{
				   std::cout<<e<<std::endl;
				}
            }
        }
        //delete mqs;
		if (!lstrData.empty())
		{
			return m_llOffSet;
		}
    }
    return m_llOffSet;
}


//发送数据
void CRocketMQ::vSendData(string strTopic, string strTag, string strKey, string strData)
{
	try
	{
		MQMessage msg(strTopic, strTag, strKey, strData);
		// 同步生产消息
		m_producer.send(msg);
	}
	catch (MQClientException& e)
	{
	   printf("[RocketMQ]数据发送失败： [topic]%s[tag]%s[key]%s[info]%s[reason]%s\n", strTopic.c_str(), strTag.c_str(), strKey.c_str(), strData.c_str(), e.what());
	}
}