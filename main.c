#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_malloc.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_net.h>
#include <rte_flow.h>

static uint8_t port_id;
static uint16_t nr_queues = 5;
struct rte_mempool *mbuf_pool;

#include "mbuf_parser.c"
#include "flow.c"

static void
main_loop(void)
{
	struct rte_mbuf *mbufs[32];
	uint16_t nb_rx;
	uint16_t i;

	while (1) {
		for (i = 0; i < nr_queues; i++) {
			nb_rx = rte_eth_rx_burst(port_id, i, mbufs, 32);
			if (nb_rx) {
				parse_mbuf(i, nb_rx, mbufs);
			}
		}
	}
}

static void
assert_link_status(void)
{
	struct rte_eth_link link;

	memset(&link, 0, sizeof(link));
	rte_eth_link_get(port_id, &link);
	if (link.link_status == ETH_LINK_DOWN) {
		rte_exit(EXIT_FAILURE, ":: error: link is still down\n");
	}
}

static void
init_port(void)
{
	int ret;
	uint16_t i;
	struct rte_eth_conf port_conf = {
		.rxmode = {
			.split_hdr_size = 0,
			.header_split   = 0, /**< Header Split disabled */
			.hw_ip_checksum = 0, /**< IP checksum offload disabled */
			.hw_vlan_filter = 0, /**< VLAN filtering disabled */
			.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
			.hw_strip_crc   = 1, /**< CRC stripped by hardware */
		},
	};

	port_conf.rx_adv_conf.rss_conf.rss_key = NULL;
	port_conf.rx_adv_conf.rss_conf.rss_hf  = ETH_RSS_IP;
	port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;

	printf(":: initializing port: %d\n", port_id);
	ret = rte_eth_dev_configure(port_id, nr_queues, nr_queues, &port_conf);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE,
			":: cannot configure device: err=%d, port=%u\n",
			ret, port_id);
	}

	/* only set Rx queues: something we care only so far */
	for (i = 0; i < nr_queues; i++) {
		ret = rte_eth_rx_queue_setup(port_id, i, 512,
				     rte_eth_dev_socket_id(port_id),
				     NULL,
				     mbuf_pool);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE,
				":: Rx queue setup failed: err=%d, port=%u\n",
				ret, port_id);
		}
	}

	if (!getenv("WITHOUT_FLOW")) {
		uint32_t intf_ip[3] = {
			0xc0a80101, /* 192.168.1.1 */
			//0xc0a80102, /* 192.168.1.2 */
			//0xc0a80103, /* 192.168.1.3 */
		};

		if (flows_setup_from_tencent(port_id, intf_ip, 1, 1) < 0)
			rte_exit(EXIT_FAILURE, "setup flow failed\n");
		else
			printf(":: flow setup is done\n");
	}

	rte_eth_promiscuous_enable(port_id);

	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE,
			"rte_eth_dev_start:err=%d, port=%u\n",
			ret, port_id);
	}

	assert_link_status();

	printf(":: initializing port: %d done\n", port_id);
}

int
main(int argc, char **argv)
{
	int ret;
	uint8_t nr_ports;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, ":: invalid EAL arguments\n");

	nr_ports = rte_eth_dev_count();
	if (nr_ports == 0)
		rte_exit(EXIT_FAILURE, ":: no Ethernet ports found\n");

	port_id = 0;
	if (nr_ports != 1) {
		printf(":: warn: %d ports deteced, but we use only one: port %u\n",
			nr_ports, port_id);
	}

	mbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", 4096, 128, 0,
					    RTE_MBUF_DEFAULT_BUF_SIZE,
					    rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	init_port();

	main_loop();

	return 0;
}
