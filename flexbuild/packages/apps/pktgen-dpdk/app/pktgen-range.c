/*-
 * Copyright (c) <2010-2017>, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of Intel Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* Created 2010 by Keith Wiles @ intel.com */

#include "pktgen-display.h"
#include "pktgen-log.h"
#include "pktgen.h"

/**************************************************************************//**
 *
 * pktgen_range_ctor - Construct a range packet in buffer provided.
 *
 * DESCRIPTION
 * Build the special range packet in the buffer provided.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */

void
pktgen_range_ctor(range_info_t *range, pkt_seq_t *pkt)
{
	switch (pkt->ethType) {
	case ETHER_TYPE_IPv4:
		switch (pkt->ipProto) {
		case PG_IPPROTO_TCP:
		case PG_IPPROTO_UDP:

			if (pkt->dport == PG_IPPROTO_L4_GTPU_PORT) {
				if (unlikely(range->gtpu_teid_inc != 0) ) {
					uint32_t teid = pkt->gtpu_teid;
					teid += range->gtpu_teid_inc;
					if (teid < range->gtpu_teid_min)
						teid = range->gtpu_teid_max;
					if (teid > range->gtpu_teid_max)
						teid = range->gtpu_teid_min;
					pkt->gtpu_teid = teid;
				} else
					pkt->gtpu_teid = range->gtpu_teid;
			}

			if (unlikely(range->src_port_inc != 0) ) {
				uint16_t sport = pkt->sport;
				sport += range->src_port_inc;
				if (sport < range->src_port_min)
					sport = range->src_port_max;
				if (sport > range->src_port_max)
					sport = range->src_port_min;
				pkt->sport = sport;
			} else
				pkt->sport = range->src_port;

			if (unlikely(range->dst_port_inc != 0) ) {
				uint16_t dport = pkt->dport;
				dport += range->dst_port_inc;
				if (dport < range->dst_port_min)
					dport = range->dst_port_max;
				if (dport > range->dst_port_max)
					dport = range->dst_port_min;
				pkt->dport = dport;
			} else
				pkt->dport = range->dst_port;

			if (unlikely(range->src_ip_inc != 0)) {
				uint32_t p = pkt->ip_src_addr.addr.ipv4.s_addr;
				p += range->src_ip_inc;
				if (p < range->src_ip_min)
					p = range->src_ip_max;
				else if (p > range->src_ip_max)
					p = range->src_ip_min;
				pkt->ip_src_addr.addr.ipv4.s_addr = p;
			} else
				pkt->ip_src_addr.addr.ipv4.s_addr =
					range->src_ip;

			if (unlikely(range->dst_ip_inc != 0)) {
				uint32_t p = pkt->ip_dst_addr.addr.ipv4.s_addr;
				p += range->dst_ip_inc;
				if (p < range->dst_ip_min)
					p = range->dst_ip_max;
				else if (p > range->dst_ip_max)
					p = range->dst_ip_min;
				pkt->ip_dst_addr.addr.ipv4.s_addr = p;
			} else
				pkt->ip_dst_addr.addr.ipv4.s_addr =
					range->dst_ip;

			if (unlikely(range->vlan_id_inc != 0)) {
				/* Since VLAN is set ot MIN_VLAN_ID, check this and skip first increment
				 * to maintina the range sequence in sync with other range fields */
				uint32_t p;
				static uint8_t min_vlan_set = 0;
				if ((pkt->vlanid == MIN_VLAN_ID) && !min_vlan_set) {
					p = 0;
					min_vlan_set = 1;
				} else
					p = pkt->vlanid;
				p += range->vlan_id_inc;
				if (p < range->vlan_id_min)
					p = range->vlan_id_max;
				else if (p > range->vlan_id_max)
					p = range->vlan_id_min;
				pkt->vlanid = p;
			} else
				pkt->vlanid = range->vlan_id;

			if (unlikely(range->cos_inc != 0)) {
				uint32_t p;
				static uint8_t min_cos_set = 0;
				if ((pkt->cos == MIN_COS) && !min_cos_set) {
					p = 0;
					min_cos_set = 1;
				} else
					p = pkt->cos;
				p += range->cos_inc;
				if (p < range->cos_min)
					p = range->cos_max;
				else if (p > range->cos_max)
					p = range->cos_min;
				pkt->cos = p;
			} else
				pkt->cos = range->cos;

			if (unlikely(range->tos_inc != 0)) {
				uint32_t p;
				static uint8_t min_tos_set = 0;
				if ((pkt->tos == MIN_TOS) && !min_tos_set) {
					p = 0;
					min_tos_set = 1;
				} else
					p = pkt->tos;
				p += range->tos_inc;
				if (p < range->tos_min)
					p = range->tos_max;
				else if (p > range->tos_max)
					p = range->tos_min;
				pkt->tos = p;
			} else
				pkt->tos = range->tos;



			if (unlikely(range->pkt_size_inc != 0)) {
				uint32_t p = pkt->pktSize;
				p += range->pkt_size_inc;
				if (p < range->pkt_size_min)
					p = range->pkt_size_max;
				else if (p > range->pkt_size_max)
					p = range->pkt_size_min;
				pkt->pktSize = p;
			} else
				pkt->pktSize = range->pkt_size;

			if (unlikely(range->src_mac_inc != 0)) {
				uint64_t p;

				inet_mtoh64(&pkt->eth_src_addr, &p);

				p += range->src_mac_inc;
				if (p < range->src_mac_min)
					p = range->src_mac_max;
				else if (p > range->src_mac_max)
					p = range->src_mac_min;

				inet_h64tom(p, &pkt->eth_src_addr);
			} else
				inet_h64tom(range->src_mac, &pkt->eth_src_addr);

			if (unlikely(range->dst_mac_inc != 0)) {
				uint64_t p;

				inet_mtoh64(&pkt->eth_dst_addr, &p);

				p += range->dst_mac_inc;
				if (p < range->dst_mac_min)
					p = range->dst_mac_max;
				else if (p > range->dst_mac_max)
					p = range->dst_mac_min;

				inet_h64tom(p, &pkt->eth_dst_addr);
			} else
				inet_h64tom(range->dst_mac, &pkt->eth_dst_addr);

			break;
		default:
			pktgen_log_info("IPv4 ipProto %02x", pkt->ipProto);
			break;
		}
		break;
	case ETHER_TYPE_IPv6:
		switch (pkt->ipProto) {
		case PG_IPPROTO_UDP:
		case PG_IPPROTO_TCP:
			/* TODO: Need to handle the IPv6 packets. */
			pktgen_log_warning("IPv6 is not yet implemented");
			break;
		default:
			pktgen_log_info("IPv6 ipProto %04x", pkt->ipProto);
			break;
		}
		break;
	default:
		pktgen_log_info("ethType %04x", pkt->ethType);
		break;
	}
}

/**************************************************************************//**
 *
 * pktgen_print_range - Display the range data page.
 *
 * DESCRIPTION
 * Display the range data page on the screen.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */

static void
pktgen_print_range(void)
{
	port_info_t *info;
	range_info_t *range;
	uint32_t pid, col, sp;
	char buff[32];
	int32_t row;
	struct ether_addr eaddr;
	char str[64];

	display_topline("<Range Page>");
	scrn_printf(1, 3, "Ports %d-%d of %d", pktgen.starting_port,
	               (pktgen.ending_port - 1), pktgen.nb_ports);

	row = PORT_STATE_ROW;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "Port #");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "dst.ip");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "src.ip");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "ip_proto");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "dst.port / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "     min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "src.port / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "     min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "vlan.id / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "cos / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "tos / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "pkt.size / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "     min / max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "dst.mac");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "src.mac");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    min");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "    max");

	row++;
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "gtpu.teid / inc");
	scrn_printf(row++, 1, "%-*s", COLUMN_WIDTH_0, "      min / max");

	/* Get the last location to use for the window starting row. */
	pktgen.last_row = ++row;
	display_dashline(pktgen.last_row);

	/* Display the colon after the row label. */
	for (row = 3; row < (pktgen.last_row - 1); row++)
		scrn_printf(row, COLUMN_WIDTH_0 - 1, ":");

	sp = pktgen.starting_port;
	for (pid = 0; pid < pktgen.nb_ports_per_page; pid++) {
		if (get_map(pktgen.l2p, pid + sp, RTE_MAX_LCORE) == 0)
			continue;

		info = &pktgen.info[pid + sp];

		/* Display Port information Src/Dest IP addr, Netmask, Src/Dst MAC addr */
		col = (COLUMN_WIDTH_1 * pid) + COLUMN_WIDTH_0;
		row = PORT_STATE_ROW;

		/* Display the port number for the column */
		snprintf(buff, sizeof(buff), "Port-%d", pid + sp);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, buff);

		range = &info->range;
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->dst_ip),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->dst_ip_inc),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->dst_ip_min),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->dst_ip_max),
		                          0xFFFFFFFF));

		row++;
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->src_ip),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->src_ip_inc),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->src_ip_min),
		                          0xFFFFFFFF));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_ntop4(buff, sizeof(buff),
		                          htonl(range->src_ip_max),
		                          0xFFFFFFFF));

		row++;
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		        (range->ip_proto == PG_IPPROTO_TCP) ? "TCP" : "UDP");

		row++;
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->dst_port,
		         range->dst_port_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->dst_port_min,
		         range->dst_port_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);

		row++;
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->src_port,
		         range->src_port_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->src_port_min,
		         range->src_port_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);

		row++;
		snprintf(str,
		         sizeof(str),
		         "%4d/%4d",
		         range->vlan_id,
		         range->vlan_id_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%4d/%4d",
		         range->vlan_id_min,
		         range->vlan_id_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);

		row++;
		snprintf(str,
		         sizeof(str),
		         "%d/%d",
		         range->cos,
		         range->cos_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%d/%d",
		         range->cos_min,
		         range->cos_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);

		row++;
		snprintf(str,
		         sizeof(str),
		         "%3d/%3d",
		         range->tos,
		         range->tos_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%3d/%3d",
		         range->tos_min,
		         range->tos_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);


		row++;
		snprintf(str,
		         sizeof(str),
		         "%4d/%4d",
		         range->pkt_size + FCS_SIZE,
		         range->pkt_size_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%4d/%4d",
		         range->pkt_size_min + FCS_SIZE,
		         range->pkt_size_max + FCS_SIZE);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);

		row++;
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->dst_mac, &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->dst_mac_inc,
		                                     &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->dst_mac_min,
		                                     &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->dst_mac_max,
		                                     &eaddr)));

		row++;
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->src_mac, &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->src_mac_inc,
		                                     &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->src_mac_min,
		                                     &eaddr)));
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1,
		               inet_mtoa(buff, sizeof(buff),
		                         inet_h64tom(range->src_mac_max,
		                                     &eaddr)));

		row++;
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->gtpu_teid,
		         range->gtpu_teid_inc);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
		snprintf(str,
		         sizeof(str),
		         "%5d/%5d",
		         range->gtpu_teid_min,
		         range->gtpu_teid_max);
		scrn_printf(row++, col, "%*s", COLUMN_WIDTH_1, str);
	}

	pktgen.flags &= ~PRINT_LABELS_FLAG;
}

/**************************************************************************//**
 *
 * pktgen_page_range - Display the range data page.
 *
 * DESCRIPTION
 * Display the range data page for a given port.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */

void
pktgen_page_range(void)
{
	if (pktgen.flags & PRINT_LABELS_FLAG)
		pktgen_print_range();
}

/**************************************************************************//**
 *
 * pktgen_range_setup - Setup the default values for a range port.
 *
 * DESCRIPTION
 * Setup the default range data for a given port.
 *
 * RETURNS: N/A
 *
 * SEE ALSO:
 */

void
pktgen_range_setup(port_info_t *info)
{
	range_info_t *range = &info->range;

	range->dst_ip       = IPv4(192, 168, info->pid + 1, 1);
	range->dst_ip_min   = IPv4(192, 168, info->pid + 1, 1);
	range->dst_ip_max   = IPv4(192, 168, info->pid + 1, 254);
	range->dst_ip_inc   = 0x00000001;

	range->src_ip       = IPv4(192, 168, info->pid, 1);
	range->src_ip_min   = IPv4(192, 168, info->pid, 1);
	range->src_ip_max   = IPv4(192, 168, info->pid, 254);
	range->src_ip_inc   = 0x00000000;

	range->ip_proto     = PG_IPPROTO_TCP;

	range->dst_port     = (info->pid << 8);
	range->dst_port_inc = 0x0001;
	range->dst_port_min = range->dst_port + 0;
	range->dst_port_max = range->dst_port + 254;

	range->src_port     = (info->pid << 8);
	range->src_port_inc = 0x0001;
	range->src_port_min = range->src_port + 0;
	range->src_port_max = range->src_port + 254;

	range->vlan_id      = info->vlanid;
	range->vlan_id_inc  = 0;
	range->vlan_id_min  = MIN_VLAN_ID;
	range->vlan_id_max  = MAX_VLAN_ID;

	range->cos   	= info->cos;
	range->cos_inc  = 0;
	range->cos_min  = MIN_COS;
	range->cos_max  = MAX_COS;

	range->tos   	= info->tos;
	range->tos_inc  = 0;
	range->tos_min  = MIN_TOS;
	range->tos_max  = MAX_TOS;

	range->pkt_size     = MIN_PKT_SIZE;
	range->pkt_size_inc = 0;
	range->pkt_size_min = MIN_PKT_SIZE;
	range->pkt_size_max = MAX_PKT_SIZE;

	info->seq_pkt[RANGE_PKT].pktSize = MIN_PKT_SIZE;

	inet_mtoh64(&info->seq_pkt[SINGLE_PKT].eth_dst_addr, &range->dst_mac);
	memset(&range->dst_mac_inc, 0, sizeof(range->dst_mac_inc));
	memset(&range->dst_mac_min, 0, sizeof(range->dst_mac_min));
	memset(&range->dst_mac_max, 0, sizeof(range->dst_mac_max));

	inet_mtoh64(&info->seq_pkt[SINGLE_PKT].eth_src_addr, &range->src_mac);
	memset(&range->src_mac_inc, 0, sizeof(range->src_mac_inc));
	memset(&range->src_mac_min, 0, sizeof(range->src_mac_min));
	memset(&range->src_mac_max, 0, sizeof(range->src_mac_max));
}
