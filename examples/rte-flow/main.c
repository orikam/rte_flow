#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>
#include <rte_net.h>
#include <rte_flow.h>
#include "flow.h"

#define RX_RING_SIZE 256
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 64
#define rxqs 4
#define txqs 4

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN, },
};
/*
 * Initialises a given port using global settings and with the rx buffers
 * coming from the mbuf_pool passed as parameter
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = rxqs, tx_rings = txqs;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count())
		return -1;

	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
						rte_eth_dev_socket_id(port),
						NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
						rte_eth_dev_socket_id(port),
						NULL);
		if (retval < 0)
			return retval;
	}
	retval  = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	return 0;
}

static void
read_packets(int nb_ports) {
	uint64_t timer_period, diff_tsc,
		cur_tsc, timer_tsc = 0, prev_tsc = 0;
	int portid, rxq;
	uint16_t nb_rx = 0; //, nb_tx = 0;
	timer_period = rte_get_timer_hz();
	for (;;) {
		for (portid = 0; portid < nb_ports; portid++) {
			struct rte_mbuf *bufs[BURST_SIZE];
			for (rxq = 0; rxq < rxqs ; rxq++) {
				prev_tsc = rte_rdtsc();
			do {
				nb_rx = rte_eth_rx_burst(portid, rxq,
						bufs, BURST_SIZE);
				cur_tsc = rte_rdtsc();
				diff_tsc = cur_tsc - prev_tsc;
				timer_tsc += diff_tsc;

				if (unlikely(nb_rx == 0))
					continue;
			} while ((nb_rx > 0) &&
				(timer_tsc < timer_period));

			if (nb_rx)
				printf("got %d packets on queue %d\n",
					nb_rx, rxq);

			uint16_t buf;
			char ptype[256];
			uint32_t sw_packet_type;
			//struct rte_net_hdr_lens hdr_lens;
			if (nb_rx) {
				for (buf = 0; buf < nb_rx; buf++) {
					sw_packet_type =
						rte_net_get_ptype(bufs[buf],
							NULL,
							RTE_PTYPE_ALL_MASK);
					rte_get_ptype_name(sw_packet_type,
							ptype, sizeof(ptype));
					printf("packet type is : %s\n", ptype);
				}
				printf("got %d packets on queue %d\n",
					nb_rx, rxq);
			}
			for (buf = 0; buf < nb_rx; buf++)
				rte_pktmbuf_free(bufs[buf]);
			}
		}
	}
}


int
main(int argc, char **argv)
{
	int ret;
	int portid;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	int nb_ports = rte_eth_dev_count();
	printf("nb_ports %d\n", nb_ports);
	struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
						NUM_MBUFS * nb_ports,
						MBUF_CACHE_SIZE, 0,
						RTE_MBUF_DEFAULT_BUF_SIZE,
						rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	/* initialize all ports */
	for (portid = 0; portid < nb_ports; portid++)
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8"\n",
				portid);


	struct rte_flow *flow = add_udp_flow(0, 3, 1432, 1234);
	if (!flow) {
		printf("Error: couldn't create UDP RTE_flow rule\n");
		return -1;
	}
	printf("UDP Flow rule created\n");

	struct rte_flow *flow1 = add_tcp_flow(0, 2, 5323, 9883);
	if (!flow1) {
		printf("Error: couldn't create TCP RTE_flow rule\n");
		return -1;
	}
	printf("TCP Flow rule created\n");

	struct rte_flow *flow2 = add_ipv4_flow(0, 1,
						0xc0a80101, 0xc0a80102, 1, 0);
	if (!flow2) {
		printf("Error: couldn't create IPV4 RTE_flow rule\n");
		return -1;
	}
	printf("IPV4 Flow rule created\n");
	uint32_t intf_ip[3] = {
		0xc0a80101, /* 192.168.1.1 */
		//0xc0a80102, /* 192.168.1.2 */
		//0xc0a80103, /* 192.168.1.3 */
		};
	(void)intf_ip;
	read_packets(nb_ports);
	return 0;
}
