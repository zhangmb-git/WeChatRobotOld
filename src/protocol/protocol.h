/** @file protocol.h
  * @brief Э�鶨��
  * @author yingchun.xu
  * @date 2020/8/14
  */

#ifndef _PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_
#define _PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_

#include "common_def.h"

// ��ѯ�ǳ�����
struct IMGetUserNickNameReq {
    uint8_t user_id[64];
};

// ��ѯ�ǳ���Ӧ
struct IMGetUserNickNameRsp {
    IMUserInfo user_info;
};

// ������Ϣ
struct IMMsgData {
    uint8_t from[64];			// Դ
    uint8_t to[64];				// Ŀ��
    IMChatMsgType msg_type;		// ��Ϣ����
    uint8_t client_msg_id[32];  // ��ϢID��ʹ��uuid
    uint32_t msg_content_len;	// ��Ϣ���ݳ���
    uint8_t msg_content[0];     // ��Ϣ����
};

// ��Ϣ�ѷ���ack
struct IMMsgAck {
    uint32_t res_code;			// ���ͽ��
    uint8_t client_msg_id[32];  // ��ϢID��ʹ��uuid
};

#endif//_PROTOCOL_052266B1_B20A_4DF9_B512_953227985839_