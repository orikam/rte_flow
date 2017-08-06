/*
 * taken from testpmd: rxonly.c
 */
static inline void
print_ether_addr(const char *what, struct ether_addr *eth_addr)
{
	char buf[ETHER_ADDR_FMT_SIZE];
	ether_format_addr(buf, ETHER_ADDR_FMT_SIZE, eth_addr);
	printf("%s%s", what, buf);
}

static void
parse_mbuf(uint16_t queue, uint16_t nb_rx, struct rte_mbuf *mbufs[])
{
	uint16_t i;
	struct ether_hdr *eth_hdr;
	uint16_t eth_type;
	uint64_t ol_flags;
	uint16_t packet_type;
	uint16_t is_encapsulation;
	char buf[256];
	struct rte_net_hdr_lens hdr_lens;
	uint32_t sw_packet_type;


	printf("queue %u: received %u packets\n", queue, nb_rx);

	for (i = 0; i < nb_rx; i++) {
		struct rte_mbuf *m = mbufs[i];

		eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
		eth_type = rte_be_to_cpu_16(eth_hdr->ether_type);
		ol_flags = m->ol_flags;
		packet_type = m->packet_type;
		is_encapsulation = RTE_ETH_IS_TUNNEL_PKT(packet_type);

		print_ether_addr("  src=", &eth_hdr->s_addr);
		print_ether_addr(" - dst=", &eth_hdr->d_addr);
		printf(" - type=0x%04x - length=%u - nb_segs=%d",
		       eth_type, (unsigned) m->pkt_len,
		       (int)m->nb_segs);
		if (ol_flags & PKT_RX_RSS_HASH) {
			printf(" - RSS hash=0x%x", (unsigned)m->hash.rss);
			printf(" - RSS queue=0x%x",(unsigned)queue);
		}
		if (ol_flags & PKT_RX_FDIR) {
			printf(" - FDIR matched ");
			if (ol_flags & PKT_RX_FDIR_ID)
				printf("ID=0x%x",
				       m->hash.fdir.hi);
			else if (ol_flags & PKT_RX_FDIR_FLX)
				printf("flex bytes=0x%08x %08x",
				       m->hash.fdir.hi, m->hash.fdir.lo);
			else
				printf("hash=0x%x ID=0x%x ",
				       m->hash.fdir.hash, m->hash.fdir.id);
		}
		if (ol_flags & PKT_RX_VLAN_STRIPPED)
			printf(" - VLAN tci=0x%x", m->vlan_tci);
		if (ol_flags & PKT_RX_QINQ_STRIPPED)
			printf(" - QinQ VLAN tci=0x%x, VLAN tci outer=0x%x",
					m->vlan_tci, m->vlan_tci_outer);
		if (m->packet_type) {
			rte_get_ptype_name(m->packet_type, buf, sizeof(buf));
			printf(" - hw ptype: %s", buf);
		}
		sw_packet_type = rte_net_get_ptype(m, &hdr_lens,
			RTE_PTYPE_ALL_MASK);
		rte_get_ptype_name(sw_packet_type, buf, sizeof(buf));
		printf(" - sw ptype: %s", buf);
		if (sw_packet_type & RTE_PTYPE_L2_MASK)
			printf(" - l2_len=%d", hdr_lens.l2_len);
		if (sw_packet_type & RTE_PTYPE_L3_MASK)
			printf(" - l3_len=%d", hdr_lens.l3_len);
		if (sw_packet_type & RTE_PTYPE_L4_MASK)
			printf(" - l4_len=%d", hdr_lens.l4_len);
		if (sw_packet_type & RTE_PTYPE_TUNNEL_MASK)
			printf(" - tunnel_len=%d", hdr_lens.tunnel_len);
		if (sw_packet_type & RTE_PTYPE_INNER_L2_MASK)
			printf(" - inner_l2_len=%d", hdr_lens.inner_l2_len);
		if (sw_packet_type & RTE_PTYPE_INNER_L3_MASK)
			printf(" - inner_l3_len=%d", hdr_lens.inner_l3_len);
		if (sw_packet_type & RTE_PTYPE_INNER_L4_MASK)
			printf(" - inner_l4_len=%d", hdr_lens.inner_l4_len);
		if (is_encapsulation) {
			struct ipv4_hdr *ipv4_hdr;
			struct ipv6_hdr *ipv6_hdr;
			struct udp_hdr *udp_hdr;
			uint8_t l2_len;
			uint8_t l3_len;
			uint8_t l4_len;
			uint8_t l4_proto;
			struct  vxlan_hdr *vxlan_hdr;

			l2_len  = sizeof(struct ether_hdr);

			 /* Do not support ipv4 option field */
			if (RTE_ETH_IS_IPV4_HDR(packet_type)) {
				l3_len = sizeof(struct ipv4_hdr);
				ipv4_hdr = rte_pktmbuf_mtod_offset(m,
								   struct ipv4_hdr *,
								   l2_len);
				l4_proto = ipv4_hdr->next_proto_id;
			} else {
				l3_len = sizeof(struct ipv6_hdr);
				ipv6_hdr = rte_pktmbuf_mtod_offset(m,
								   struct ipv6_hdr *,
								   l2_len);
				l4_proto = ipv6_hdr->proto;
			}
			if (l4_proto == IPPROTO_UDP) {
				udp_hdr = rte_pktmbuf_mtod_offset(m,
								  struct udp_hdr *,
								  l2_len + l3_len);
				l4_len = sizeof(struct udp_hdr);
				vxlan_hdr = rte_pktmbuf_mtod_offset(m,
								    struct vxlan_hdr *,
								    l2_len + l3_len + l4_len);

				printf(" - VXLAN packet: packet type =%d, "
					"Destination UDP port =%d, VNI = %d",
					packet_type, rte_be_to_cpu_16(udp_hdr->dst_port),
					rte_be_to_cpu_32(vxlan_hdr->vx_vni) >> 8);
			}
		}
		printf(" - Receive queue=0x%x", (unsigned)queue);
		printf("\n");
		rte_get_rx_ol_flag_list(m->ol_flags, buf, sizeof(buf));
		printf("  ol_flags: %s\n", buf);
		rte_pktmbuf_free(m);
	}
}
