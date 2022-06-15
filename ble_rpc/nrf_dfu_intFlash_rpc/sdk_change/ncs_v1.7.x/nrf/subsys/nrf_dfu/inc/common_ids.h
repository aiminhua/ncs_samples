/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef COMMON_IDS_H_
#define COMMON_IDS_H_

#ifdef __cplusplus
extern "C" {
#endif

enum rpc_command {
	RPC_CMD_NRF_DFU_REQ_HANDLER_ON_REQ = 1,
	RPC_CMD_DFU_REQ_HANDLER_CALLBACK = 2,
	RPC_COMMAND_APP_SEND,
	RPC_COMMAND_NET_SEND,	
	RPC_COMMAND_APP_BT_NUS_SEND,
	RPC_COMMAND_NET_BT_NUS_RECEIVE_CB,
	RPC_COMMAND_APP_BT_SMP_SEND,
	RPC_COMMAND_NET_BT_SMP_RECEIVE_CB,
	RPC_COMMAND_APP_BT_SMP_GET_MTU 	
};

enum rpc_api_type {
	NET2APP_BT_ADDR_SEND = 0x01,
	APP2NET_BT_NUS_SEND,
	APP2NET_BT_SMP_SEND,
	NET2APP_BT_NUS_RECV,
	NET2APP_BT_CONN_STATUS
/*  RPC_COMMAND_NET_BT_MTU_SIZE_CB = 0x05, */
};

#ifdef __cplusplus
}
#endif

#endif /* COMMON_IDS_H_ */
