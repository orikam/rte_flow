/*
 * flow_blocks.c
 *
 *  Created on: Aug 9, 2017
 *      Author: Ori Kam
 */



#define MAX_PATTERN_NUM		4

static struct rte_flow *
generate_ipv4_rule(uint8_t port_id, uint16_t rx_q,
		uint32_t src_ip, uint32_t src_mask,
		uint32_t dest_ip, uint32_t dest_mask) {

	struct rte_flow_attr attr;
	struct rte_flow_item pattern[MAX_PATTERN_NUM];
	struct rte_flow_action action[MAX_PATTERN_NUM];
	struct rte_flow *flow;
	struct rte_flow_error error;
//	int nr_queues = 5;

	/* Let's isolate "rx_q" first, to block other pkts enter it */
//	isolate_queue(port, rx_q, nr_queues);

	src_ip = src_ip; //remove warnings
	src_mask = src_mask; //remove warnings
	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));

	// Action
	/*
	 * create one action only,  move packet to queue
	 */
	struct rte_flow_action_queue queue = { .index = rx_q };

	action[0].type= RTE_FLOW_ACTION_TYPE_QUEUE;
	action[0].conf = &queue;

	action[1].type= RTE_FLOW_ACTION_TYPE_COUNT;

	action[2].type= RTE_FLOW_ACTION_TYPE_END;

	/*
	 * set the rule attribute.
	 * in this case only ingress packets will be checked.
	 */
	memset(&attr, 0, sizeof(struct rte_flow_attr));
	attr.ingress = 1;

	/*
	 * set the first level of the pattern.
	 * since in this example we just want to get the
	 * ipv4 we just set it.
	 */

	struct rte_flow_item_eth eth_spec;
	struct rte_flow_item_eth eth_mask;

	memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
	memset(&eth_mask, 0, sizeof(struct rte_flow_item_eth));
	eth_spec.type = 0;
	eth_mask.type = 0;

	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_spec;
	pattern[0].mask = &eth_mask;


	// vlan
	struct rte_flow_item_vlan vlan_spec;
	struct rte_flow_item_vlan vlan_mask;

	memset(&vlan_spec, 0, sizeof(struct rte_flow_item_vlan));
	memset(&vlan_mask, 0, sizeof(struct rte_flow_item_vlan));
	/* match all VLAN tags */
//	vlan_spec.tci = htons(0x123);
//	vlan_mask.tci = 0xffff;

	pattern[1].type = RTE_FLOW_ITEM_TYPE_VLAN;
	pattern[1].spec = &vlan_spec;
	pattern[1].mask = &vlan_mask;



	struct rte_flow_item_ipv4 ip_spec;
	struct rte_flow_item_ipv4 ip_mask;

	memset(&ip_spec,0, sizeof(struct rte_flow_item_ipv4));
	memset(&ip_mask,0, sizeof(struct rte_flow_item_ipv4));
	ip_spec.hdr.dst_addr = htonl(dest_ip);
	ip_mask.hdr.dst_addr = dest_mask;

	pattern[2].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[2].spec = &ip_spec;
	pattern[2].mask = &ip_mask;


	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;

	int res = rte_flow_validate(port_id, &attr, pattern, action, &error);
	printf("\nrule validation result = %d\n",res);
	flow = rte_flow_create(port_id, &attr, pattern, action, &error);

	return flow;

}


//
//static struct rte_flow *
//generate_vlan_ipv4_rule(uint8_t port_id, uint16_t rx_q,
//		uint32_t vlan, uint32_t vlan_mask,
//		uint32_t src_ip, uint32_t src_mask,
//		uint32_t dest_ip, uint32_t dest_mask) {
//
//	struct rte_flow_attr attr;
//	struct rte_flow_item pattern[MAX_PATTERN_NUM];
//	struct rte_flow_action action[MAX_PATTERN_NUM];
//	struct rte_flow *flow;
//	struct rte_flow_error error;
////	int nr_queues = 5;
//
//	/* Let's isolate "rx_q" first, to block other pkts enter it */
////	isolate_queue(port, rx_q, nr_queues);
//
//	src_ip = src_ip; //remove warnings
//	src_mask = src_mask; //remove warnings
//	memset(pattern, 0, sizeof(pattern));
//	memset(action, 0, sizeof(action));
//
//	// Action
//	/*
//	 * create one action only,  move packet to queue
//	 */
//	struct rte_flow_action_queue queue = { .index = rx_q };
//
//	action[0].type= RTE_FLOW_ACTION_TYPE_QUEUE;
//	action[0].conf = &queue;
//
//	action[1].type= RTE_FLOW_ACTION_TYPE_END;
//
//	/*
//	 * set the rule attribute.
//	 * in this case only ingress packets will be checked.
//	 */
//	memset(&attr, 0, sizeof(struct rte_flow_attr));
//	attr.ingress = 1;
//
//	/*
//	 * set the first level of the pattern.
//	 * since in this example we just want to get the
//	 * ipv4 we just set it.
//	 */
//
//	struct rte_flow_item_eth eth_spec;
//	struct rte_flow_item_eth eth_mask;
//
//	memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
//	memset(&eth_mask, 0, sizeof(struct rte_flow_item_eth));
//	eth_spec.type = 0;
//	eth_mask.type = 0;
//
//	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
//	pattern[0].spec = &eth_spec;
//	pattern[0].mask = &eth_mask;
//
//
//	// vlan
//	struct rte_flow_item_vlan vlan_spec;
//	struct rte_flow_item_vlan vlan_mask;
//
//	memset(&vlan_spec, 0, sizeof(struct rte_flow_item_vlan));
//	memset(&vlan_mask, 0, sizeof(struct rte_flow_item_vlan));
//	/* match all VLAN tags */
//	//vlan_spec.tci = htons(0x123);
//	//vlan_mask.tci = 0xffff;
//
//	pattern[1].type = RTE_FLOW_ITEM_TYPE_VLAN;
//	pattern[1].spec = &vlan_spec;
//	pattern[1].mask = &vlan_mask;
//
//
//
//	struct rte_flow_item_ipv4 ip_spec;
//	struct rte_flow_item_ipv4 ip_mask;
//
//	memset(&ip_spec,0, sizeof(struct rte_flow_item_ipv4));
//	memset(&ip_mask,0, sizeof(struct rte_flow_item_ipv4));
//	ip_spec.hdr.dst_addr = htonl(dest_ip);
//	ip_mask.hdr.dst_addr = dest_mask;
//
//	pattern[2].type = RTE_FLOW_ITEM_TYPE_IPV4;
//	pattern[2].spec = &ip_spec;
//	pattern[2].mask = &ip_mask;
//
//	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;
//
//	flow = rte_flow_create(port_id, &attr, pattern, action, &error);
//
//	return flow;
//
//}






