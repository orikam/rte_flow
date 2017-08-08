#include <rte_ethdev.h>
#include <rte_flow.h>
#include <rte_net.h>
#include "flow.h"

struct rte_flow_attr flow_attr = {
	.group = 0,
	.priority = 0,
	.ingress = 1,
	.egress = 0,
	.reserved = 0,
};
struct rte_flow_error flow_err;
struct rte_flow_item_eth eth_item;
struct rte_flow_item_eth eth_mask;
struct rte_flow_action_queue action;
struct rte_flow_action actions[2];

static void
prepare_eth_part(uint8_t port_id) {
	rte_eth_macaddr_get(port_id, &eth_item.dst);
	printf("\neth_item.dst = %02"PRIx8" %02"PRIx8" %02"PRIx8
		" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n\n",
		eth_item.dst.addr_bytes[0], eth_item.dst.addr_bytes[1],
		eth_item.dst.addr_bytes[2], eth_item.dst.addr_bytes[3],
		eth_item.dst.addr_bytes[4], eth_item.dst.addr_bytes[5]);
	memset(&eth_mask, 0, sizeof(eth_mask));
	memset(eth_mask.dst.addr_bytes, 0xff, ETHER_ADDR_LEN);
}

static void
prepare_queue_action(uint16_t q_id) {
	action.index = q_id;
	actions[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
	actions[0].conf = &action;
	actions[1].type = RTE_FLOW_ACTION_TYPE_END;
}
/*
 * ipv4 flow rules only support 0 or full mask
 */
struct rte_flow*
add_ipv4_flow(uint8_t port_id, uint16_t q_id,
		uint32_t src_ip, uint32_t dst_ip,
		uint32_t src_mask, uint32_t dst_mask) {
	int rc;
	struct rte_flow_item_ipv4 ipv4_item;
	struct rte_flow_item_ipv4 ipv4_mask;
	struct rte_flow_item pattern[3];
	struct rte_flow *flow;
	uint32_t mask;

	prepare_eth_part(port_id);

	ipv4_item.hdr.src_addr = htonl(src_ip);
	ipv4_item.hdr.dst_addr = htonl(dst_ip);

	mask = (src_mask ? 0xff : src_mask);
	memset(&ipv4_mask.hdr.src_addr, mask, sizeof(ipv4_mask.hdr.src_addr));

	mask = (dst_mask ? 0xff : dst_mask);

	memset(&ipv4_mask.hdr.dst_addr, mask, sizeof(ipv4_mask.hdr.dst_addr));

	prepare_queue_action(q_id);

	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_item;
	pattern[0].last = &eth_item;
	pattern[0].mask = &eth_mask;

	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[1].spec = &ipv4_item;
	pattern[1].last = &ipv4_item;
	pattern[1].mask = &ipv4_mask;

	pattern[2].type = RTE_FLOW_ITEM_TYPE_END;
	rc = rte_flow_validate(port_id, &flow_attr,
				pattern, actions, &flow_err);

	if (rc) {
		printf("error %d ipv4 flow validation failed. msg: %s\n",
			rc, flow_err.message);
		printf("flow error type %d\n", flow_err.type);
		return NULL;
	}

	flow = rte_flow_create(port_id, &flow_attr,
				pattern, actions, &flow_err);

	return flow;
}

struct rte_flow*
add_tcp_flow(uint8_t port_id, uint16_t q_id,
		uint16_t src_port, uint16_t dst_port) {
	struct rte_flow_item_ipv4 ipv4_item;
	struct rte_flow_item_ipv4 ipv4_mask;
	struct rte_flow_item_tcp tcp_item;
	struct rte_flow_item_tcp tcp_mask;
	struct rte_flow_item pattern[4];
	struct rte_flow *flow;

	prepare_eth_part(port_id);

	memset(&ipv4_mask, 0, sizeof(ipv4_mask));
	tcp_item.hdr.src_port = htons(src_port);
	tcp_item.hdr.dst_port =  htons(dst_port);
	memset(&tcp_mask, 0xff, sizeof(tcp_mask));

	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_item;
	pattern[0].last = &eth_item;
	pattern[0].mask = &eth_mask;

	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[1].spec = &ipv4_item;
	pattern[1].last = &ipv4_item;
	pattern[1].mask = &ipv4_mask;

	pattern[2].type = RTE_FLOW_ITEM_TYPE_TCP;
	pattern[2].spec = &tcp_item;
	pattern[2].last = &tcp_item;
	pattern[2].mask = &tcp_mask;
	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;

	prepare_queue_action(q_id);

	int rc = rte_flow_validate(port_id, &flow_attr,
					pattern, actions, &flow_err);
	if (rc) {
		printf("error %d tcp flow validation failed. msg: %s\n",
			rc, flow_err.message);
		printf("flow error type %d\n", flow_err.type);
		return NULL;
	}

	flow = rte_flow_create(port_id, &flow_attr,
				pattern, actions, &flow_err);
	if (!flow) {
		printf("error %d tcp flow creation failed. msg: %s\n",
		rte_errno, flow_err.message);
		printf("flow error type %d\n", flow_err.type);
	}

	return flow;
}

struct rte_flow*
add_udp_flow(uint8_t port_id, uint16_t q_id,
		uint16_t src_port, uint16_t dst_port) {
	struct rte_flow_item_ipv4 ipv4_item;
	struct rte_flow_item_ipv4 ipv4_mask;
	struct rte_flow_item_udp udp_item;
	struct rte_flow_item_udp udp_mask;
	struct rte_flow_item pattern[4];
	struct rte_flow *flow;

	prepare_eth_part(port_id);

	memset(&ipv4_mask, 0, sizeof(ipv4_mask));
	udp_item.hdr.src_port = htons(src_port);
	udp_item.hdr.dst_port =  htons(dst_port);
	memset(&udp_mask, 0xff, sizeof(udp_mask));

	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_item;
	pattern[0].last = &eth_item;
	pattern[0].mask = &eth_mask;

	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[1].spec = &ipv4_item;
	pattern[1].last = &ipv4_item;
	pattern[1].mask = &ipv4_mask;

	pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
	pattern[2].spec = &udp_item;
	pattern[2].last = &udp_item;
	pattern[2].mask = &udp_mask;

	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;

	prepare_queue_action(q_id);

	int rc = rte_flow_validate(port_id, &flow_attr,
					pattern, actions, &flow_err);
	if (rc) {
		printf("error %d udp flow validation failed. msg: %s\n",
			rc, flow_err.message);
		printf("flow error type %d\n", flow_err.type);
		return NULL;
	}

	flow = rte_flow_create(port_id, &flow_attr,
						pattern, actions, &flow_err);
	if (!flow) {
		printf("error %d udp flow creation failed. msg: %s\n",
			rte_errno, flow_err.message);
		printf("flow error type %d\n", flow_err.type);
	}
	return flow;
}
