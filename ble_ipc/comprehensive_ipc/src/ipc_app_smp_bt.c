/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <errno.h>
#include <zephyr/init.h>
#include "../../ipc_cmd_ids.h"
#include "ipc_app_smp_bt.h"
#include <zephyr/mgmt/mcumgr/smp.h>
#include "smp_reassembly.h"
#include "smp/smp.h"
#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>
#include "ipc_app_api.h"

LOG_MODULE_REGISTER(ipc_app_smp, 3);
static struct smp_transport smp_ipc_transport;
static uint16_t mtu;

int smp_receive_data(const void *buf, uint16_t len)
{
	LOG_HEXDUMP_DBG(buf, len, "ipc app smp rx:");

#ifdef CONFIG_IPC_REASSEMBLY_BT
	int ret;
	bool started;

	started = (smp_reassembly_expected(&smp_ipc_transport) >= 0);
	ret = smp_reassembly_collect(&smp_ipc_transport, buf, len);

	LOG_DBG("collect = %d", ret);

	/*
	 * Collection can fail only due to failing to allocate memory or by receiving
	 * more data than expected.
	 */
	if (ret == -ENOMEM) {
		/* Failed to collect the buffer */
		return -ENOMEM;
	} else if (ret < 0) {
		smp_reassembly_drop(&smp_ipc_transport);
		return ret;
	}

	/* No more bytes are expected for this packet */
	if (ret == 0) {
		smp_reassembly_complete(&smp_ipc_transport, false);
	}

	return 0;
#else
	struct net_buf *nb;	

	nb = smp_packet_alloc();
	if (!nb)
	{
		LOG_ERR("net buf alloc failed");
		return -ENOMEM;
	}
	net_buf_add_mem(nb, buf, len);
	smp_rx_req(&smp_ipc_transport, nb);	

	return 0;
#endif	
}

/**
 * Transmits the specified SMP response.
 */
static int smp_ipc_tx_pkt(struct net_buf *nb)
{	
	int rc;
	LOG_HEXDUMP_DBG(nb->data, nb->len, "ipc app send");
	rc = app2net_smp_send(nb->data, nb->len);
	smp_packet_free(nb);
	return rc;
}

void set_smp_mtu(uint16_t len)
{
	mtu = len;
}

static uint16_t smp_ipc_get_mtu(const struct net_buf *nb)
{
	if (mtu == 0)
	{
		app2net_smp_get_mtu();
		k_msleep(5);
		LOG_INF("smp mtu:%d", mtu);	
	}
	
	return mtu;     
}

int smp_ipc_init(const struct device *dev)
{	
	ARG_UNUSED(dev);
	
	smp_transport_init(&smp_ipc_transport, smp_ipc_tx_pkt,
				  smp_ipc_get_mtu, NULL,
				  NULL, NULL);
	return 0;
}

SYS_INIT(smp_ipc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
