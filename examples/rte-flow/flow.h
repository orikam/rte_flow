#ifndef _FLOW_
#define _FLOW_
#include <rte_flow.h>

struct rte_flow*
add_ipv4_flow(uint8_t port_id, uint16_t q_id,
		uint32_t src_ip, uint32_t dst_ip,
		uint32_t src_mask, uint32_t dst_mask);

struct rte_flow*
add_tcp_flow(uint8_t port_id, uint16_t q_id,
		uint16_t src_port, uint16_t dst_port);

struct rte_flow*
add_udp_flow(uint8_t port_id, uint16_t q_id,
		uint16_t src_port, uint16_t dst_port);

#endif /* _FLOW_ */
