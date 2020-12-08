/** @file protocol.h
  * @brief 协议定义
  * @author yingchun.xu
  * @date 2020/8/14
  */

#ifndef _PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_
#define _PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_

#include "common_def.h"

// 查询昵称请求
struct IMGetUserNickNameReq {
    uint8_t user_id[64];
};

// 查询昵称响应
struct IMGetUserNickNameRsp {
    IMUserInfo user_info;
};

// 发送消息
struct IMMsgData {
    uint8_t from[64];			// 源
    uint8_t to[64];				// 目标
    IMChatMsgType msg_type;		// 消息类型
    uint8_t client_msg_id[32];  // 消息ID，使用uuid
    uint32_t msg_content_len;	// 消息内容长度
    uint8_t msg_content[0];     // 消息内容
};

// 消息已发送ack
struct IMMsgAck {
    uint32_t res_code;			// 发送结果
    uint8_t client_msg_id[32];  // 消息ID，使用uuid
};

#endif//_PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_