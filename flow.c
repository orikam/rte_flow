/* From testpmd */

static int
port_flow_complain(struct rte_flow_error *error)
{
	static const char *const errstrlist[] = {
		[RTE_FLOW_ERROR_TYPE_NONE] = "no error",
		[RTE_FLOW_ERROR_TYPE_UNSPECIFIED] = "cause unspecified",
		[RTE_FLOW_ERROR_TYPE_HANDLE] = "flow rule (handle)",
		[RTE_FLOW_ERROR_TYPE_ATTR_GROUP] = "group field",
		[RTE_FLOW_ERROR_TYPE_ATTR_PRIORITY] = "priority field",
		[RTE_FLOW_ERROR_TYPE_ATTR_INGRESS] = "ingress field",
		[RTE_FLOW_ERROR_TYPE_ATTR_EGRESS] = "egress field",
		[RTE_FLOW_ERROR_TYPE_ATTR] = "attributes structure",
		[RTE_FLOW_ERROR_TYPE_ITEM_NUM] = "pattern length",
		[RTE_FLOW_ERROR_TYPE_ITEM] = "specific pattern item",
		[RTE_FLOW_ERROR_TYPE_ACTION_NUM] = "number of actions",
		[RTE_FLOW_ERROR_TYPE_ACTION] = "specific action",
	};
	const char *errstr;
	char buf[32];
	int err = rte_errno;

	if ((unsigned int)error->type >= RTE_DIM(errstrlist) ||
	    !errstrlist[error->type])
		errstr = "unknown type";
	else
		errstr = errstrlist[error->type];
	printf("Caught error type %d (%s): %s%s\n",
			error->type, errstr,
			error->cause ? (snprintf(buf, sizeof(buf), "cause: %p, ",
					error->cause), buf) : "",
			error->message ? error->message : "(no stated reason)");
	return -err;
}

static uint16_t
next_queue_id(uint16_t start, uint16_t isolated_queue, uint16_t nr_queues)
{
	start = start % nr_queues;
	if (start == isolated_queue)
		start += 1;

	return start % nr_queues;
}

static void query_reta(uint8_t port_id)
{
	struct rte_eth_dev_info dev_info;
	struct rte_eth_rss_reta_entry64 reta_conf[8];
	uint16_t i, idx, off;

	printf(":: querying RETA ...\n");

	memset(&dev_info, 0, sizeof(dev_info));
	rte_eth_dev_info_get(port_id, &dev_info);

	for (i = 0; i < 8; i++) {
		reta_conf[i].mask = 0xffffffff;
	}
	rte_eth_dev_rss_reta_query(port_id, reta_conf, dev_info.reta_size);
	for (i = 0; i < dev_info.reta_size; i++) {
		idx = i / RTE_RETA_GROUP_SIZE;
		off = i % RTE_RETA_GROUP_SIZE;
		if (!(reta_conf[idx].mask & (1ULL << off)))
			continue;

		printf("<- rss reta index: %u, queue: %u\n",
			i, reta_conf[idx].reta[off]);
	}
}

static void
set_reta(struct rte_eth_rss_reta_entry64 *reta_conf,
	 uint16_t reta_index, uint16_t queue)
{
	uint16_t idx, off;

	idx = reta_index / RTE_RETA_GROUP_SIZE;
	off = reta_index % RTE_RETA_GROUP_SIZE;

	reta_conf[idx].mask |= (1ULL << off);
	reta_conf[idx].reta[off] = queue;

	printf("-> rss reta index: %u, queue: %u\n", reta_index, queue);
}

static void
isolate_queue(uint8_t port_id, uint8_t queue, uint16_t nr_queues)
{
	struct rte_eth_dev_info dev_info;
	struct rte_eth_rss_reta_entry64 reta_conf[8];
	uint16_t reta_size;

	printf(":: setting RETA ...\n");

	memset(&dev_info, 0, sizeof(dev_info));
	rte_eth_dev_info_get(port_id, &dev_info);
	reta_size = dev_info.reta_size;

	/*
	 * if the left queue number is power of 2, we then
	 * set the RETA size to (nr_queues - 1), so that it
	 * could be a perfect hash.
	 *
	 * Of course, it should be less than the firstly
	 * configured RETA size.
	 */
	if (((nr_queues - 1) & (nr_queues - 1 - 1)) == 0)
		reta_size = RTE_MIN(nr_queues - 1, reta_size);

	uint16_t i;
	uint16_t q_id = next_queue_id(0, queue, nr_queues);
	for (i = 0; i < reta_size; i++) {
		set_reta(reta_conf, i, q_id);
		q_id = next_queue_id(q_id + 1, queue, nr_queues);
	}

	if (rte_eth_dev_rss_reta_update(port_id, reta_conf, reta_size)) {
		printf(":: ret update failed!\n");
		return;
	}

	/* just want to do a dobule check that reta sets correctly */
	query_reta(port_id);
}

/* Edited from Tencent */

/*
 * Notes: following fields has to be in network endian
 *        - Ether type
 *        - vlan tag
 *        - ip address
 */

#define MAX_PATTERN_NUM		4
static int
flows_setup_from_tencent(uint8_t port_id, uint32_t *intf_ip,
			 uint32_t ip_count, uint8_t rx_q)
{
	struct rte_flow_attr attr;
	struct rte_flow_item pattern[MAX_PATTERN_NUM];
	struct rte_flow_action action[MAX_PATTERN_NUM];
	struct rte_flow *flow;
	struct rte_flow_error error;

	/* Let's isolate "rx_q" first, to block other pkts enter it */
	isolate_queue(port_id, rx_q, nr_queues);

	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));

	// Action
	/*
	 * create one action only,  since we literally just have one
	 * action: queue matched patches to "rx_q".
	 */
	struct rte_flow_action_queue queue = { .index = rx_q };

	action[0].type= RTE_FLOW_ACTION_TYPE_QUEUE;
	action[0].conf = &queue;

	action[1].type= RTE_FLOW_ACTION_TYPE_END;


	memset(&attr, 0, sizeof(struct rte_flow_attr));
	attr.ingress = 1;

	// Ether
	struct rte_flow_item_eth eth_spec;
	struct rte_flow_item_eth eth_mask;

	memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
	memset(&eth_mask, 0, sizeof(struct rte_flow_item_eth));
	eth_spec.type = htons(ETHER_TYPE_ARP);
	eth_mask.type = 0xffff;

	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	pattern[0].spec = &eth_spec;
	pattern[0].mask = &eth_mask;
	flow = rte_flow_create(port_id, &attr, pattern, action, &error);
	if (!flow)
		return port_flow_complain(&error);

	/*
	 * NOTE: we can't set VLAN type here, otherwise the flow
	 *       won't be able to be created. Because in the MLX
	 *       implementation (which is then implemented through
	 *       ibverbs), the vlan and eth share one structure.
	 *       That means there is only one type can be specified.
	 *
	 *       If type is really wanted, it should be ETHER_TYPE_IPv4
	 *       but not ETHER_TYPE_VLAN.
	 *
	 *       With the below two lines being commented, we are
	 *       basically creating a NULL flow for eth: match all
	 *       eth packets.
	 */
	//eth_spec.type = htons(ETHER_TYPE_VLAN);
	//eth_mask.type = 0xffff;

	// reset for IP flows
	eth_spec.type = 0;
	eth_mask.type = 0;

	// vlan
	struct rte_flow_item_vlan vlan_spec;
	struct rte_flow_item_vlan vlan_mask;

	memset(&vlan_spec, 0, sizeof(struct rte_flow_item_vlan));
	memset(&vlan_mask, 0, sizeof(struct rte_flow_item_vlan));
	/* match all VLAN tags */
	//vlan_spec.tci = htons(0x123);
	//vlan_mask.tci = 0xffff;

	pattern[1].type = RTE_FLOW_ITEM_TYPE_VLAN;
	pattern[1].spec = &vlan_spec;
	pattern[1].mask = &vlan_mask;


	uint32_t i = 0;
	struct rte_flow_item_ipv4 ip_spec;
	struct rte_flow_item_ipv4 ip_mask;

	memset(&ip_spec,0, sizeof(struct rte_flow_item_ipv4));
	memset(&ip_mask,0, sizeof(struct rte_flow_item_ipv4));
	for (i = 0;i<ip_count;i++) {
		ip_spec.hdr.dst_addr = htonl(intf_ip[i]);
		ip_mask.hdr.dst_addr = 0xffffffff;

		pattern[2].type = RTE_FLOW_ITEM_TYPE_IPV4;
		pattern[2].spec = &ip_spec;
		pattern[2].mask = &ip_mask;

		pattern[3].type = RTE_FLOW_ITEM_TYPE_END;

		flow = rte_flow_create(port_id, &attr, pattern, action, &error);
		if (!flow)
			return port_flow_complain(&error);
	}

	return 0;
}
