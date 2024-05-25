
#ifndef	FASTBAT_BATMAN_H
#define	FASTBAT_BATMAN_H

#include <linux/jhash.h>
#include <linux/list.h>
#include <linux/ratelimit.h>
#include <linux/if_vlan.h>
//#include "packet.h"
#include <linux/etherdevice.h>
#include "soft-interface.h"
#include "types.h"
#include "hash.h"

#define FASTBAT_DBG			1			/* debug mode: [0..2] */
#define PUSH_THE_PACE		0
//#define ETH_ALEN			6
#define ETH_VLAN_HLEN		18
#define FAST_BAT_LIST_LEN	16

typedef struct	fast_bat {
	struct hlist_node	list;
	u8	src1[ETH_ALEN];					//1st-layer src
	u8	dst1[ETH_ALEN];					//1st-layer dst
	u8	src[ETH_ALEN];					//real src
	u8	dst[ETH_ALEN];					//real dst
	u8	orig[ETH_ALEN];
	u8	dirty;
	unsigned long	last_seen;			//time of last handle
	u32	count;
	struct net_device	*soft_iface;	//batman iface
	struct net_device	*net_dev;		//output iface
	u8	vlan;							// [0/1] => [yes/no]
	struct vlan_hdr	vlan_data;
	u8	batman;							// [0/1] => [yes/no]
	u8	batman_type;					// [0/BATADV_UNICAST/BATADV_UNICAST_4ADDR]
	union batman_header	{
		struct batadv_unicast_packet		u;
		struct batadv_unicast_4addr_packet	u4;
	}bat_data;
}FAST_BAT;

typedef struct	fast_local {
	struct list_head	list;
	u8	dst[ETH_ALEN];
	unsigned long	last_seen;
	u8	dirty;
	u8	vlan;							//vlan is on or not
	struct net_device	*output_dev;
}FAST_LOCAL;

int			fastbat_dispatch(struct sk_buff *, FAST_BAT *, u16, u16);
int			fastbat_filter_input_packet(struct sk_buff *, u16 *, u16 *);
FAST_BAT	*fastbat_list_search(u8 *, u8 *);
void		fastbat_show_raw_data(u8 *, u8);
FAST_LOCAL	*fastbat_local_search(u8 *);
int			fastbat_chk_and_add_local_entry(struct sk_buff *, struct net_device *);
#endif	//FASTBAT_BATMAN_H

#if	0
struct vlan_ethhdr {
	unsigned char	h_dest[ETH_ALEN];
	unsigned char	h_source[ETH_ALEN];
	__be16		h_vlan_proto;
	__be16		h_vlan_TCI;
	__be16		h_vlan_encapsulated_proto;
};
 33 /*
 34  *  struct vlan_hdr - vlan header
 35  *  @h_vlan_TCI: priority and VLAN ID
 36  *  @h_vlan_encapsulated_proto: packet type ID or len
 37  */
 38 struct vlan_hdr {
 39     __be16  h_vlan_TCI;
 40     __be16  h_vlan_encapsulated_proto;
 41 };

387 /**
388  * struct batadv_unicast_packet - unicast packet for network payload
389  * @packet_type: batman-adv packet type, part of the general header
390  * @version: batman-adv protocol version, part of the genereal header
391  * @ttl: time to live for this packet, part of the genereal header
392  * @ttvn: translation table version number
393  * @dest: originator destination of the unicast packet
394  */
395 struct batadv_unicast_packet {
396     u8 packet_type;
397     u8 version;
398     u8 ttl;
399     u8 ttvn; /* destination translation table version number */
400     u8 dest[ETH_ALEN];
401     /* "4 bytes boundary + 2 bytes" long to make the payload after the
402      * following ethernet header again 4 bytes boundary aligned
403      */
404 };
405
406 /*
407  * struct batadv_unicast_4addr_packet - extended unicast packet
408  * @u: common unicast packet header
409  * @src: address of the source
410  * @subtype: packet subtype
411  * @reserved: reserved byte for alignment
412  */
413 struct batadv_unicast_4addr_packet {
414     struct batadv_unicast_packet u;
415     u8 src[ETH_ALEN];
416     u8 subtype;
417     u8 reserved;
418     /* "4 bytes boundary + 2 bytes" long to make the payload after the
419      * following ethernet header again 4 bytes boundary aligned
420      */
421 };
#endif
