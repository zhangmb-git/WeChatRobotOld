/*================================================================
 *   Copyright (C) 2014 All rights reserved.
 *
 *   文件名称：MessageContent.h
 *   创 建 者：Zhang Yuanhao
 *   邮    箱：bluefoxah@gmail.com
 *   创建日期：2014年12月15日
 *   描    述：
 *
 ================================================================*/

#pragma  once


#include <iostream>
#include <string>
#include <list>

#include "DefaultMQPullConsumer.h"
#include "DefaultMQProducer.h"

using namespace rocketmq;
using namespace std;

class CRocketMQ
{
public:
//consumer
CRocketMQ(string strAdd, long long llset);

//productor
CRocketMQ(string strAdd);
~CRocketMQ();

public:
//consumer接收消息，接收前需设置set（init）
long long lgetData(string strTopic, list<string>& lstrData);

//发送消息
void vSendData(string strTopic, string strTag, string strKey, string strData);

private:
//consumer初始化
void vInit(string strAdd, long long llset); 
//productor初始化
void vInit(string strAdd); 

private:
DefaultMQPullConsumer m_consumer;
DefaultMQProducer m_producer;
long long m_llOffSet;
bool m_bConsumerFlag;
bool m_bProducerFlag;

};



