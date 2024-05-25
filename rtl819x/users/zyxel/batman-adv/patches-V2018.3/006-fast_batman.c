#include "fast_batman.h"

#define FASTBAT_MASSAGE_RATE_LIMIT		(10*HZ)
#define FASTBAT_WARN_RATE_LIMIT			(6*HZ)
#define FASTBAT_ERR_RATE_LIMIT			(1*HZ)

DEFINE_RATELIMIT_STATE(fastbat_message_state, FASTBAT_MASSAGE_RATE_LIMIT, 1);
DEFINE_RATELIMIT_STATE(fastbat_warn_state, FASTBAT_WARN_RATE_LIMIT, 1);
DEFINE_RATELIMIT_STATE(fastbat_bug_state, FASTBAT_WARN_RATE_LIMIT, 1);
DEFINE_RATELIMIT_STATE(fastbat_err_state, FASTBAT_WARN_RATE_LIMIT, 3);

#if	(FASTBAT_DBG==1)
#define FASTBAT_DBG_RAW_MSG(...)				printk(KERN_WARNING __VA_ARGS__)

#define FASTBAT_DBG_MSG(...)					\
do {											\
	if(__ratelimit(&fastbat_message_state))		\
		printk(KERN_WARNING "[F@STBAT]" __VA_ARGS__);	\
} while(0)
#define xxxFASTBAT_DBG_RAW_MSG(...)
#define xxxFASTBAT_DBG_MSG(...)
#define FASTBAT_ERR_MSG(...)					\
do {											\
	if(__ratelimit(&fastbat_err_state))			\
		printk(KERN_ERR "[F@STBAT]" __VA_ARGS__);	\
} while(0)
#else	//!FASTBAT_DBG
#define FASTBAT_DBG_RAW_MSG(...)
#define xxxFASTBAT_DBG_RAW_MSG(...)
#define FASTBAT_DBG_MSG(...)
#define xxxFASTBAT_DBG_MSG(...)
#define FASTBAT_ERR_MSG(...)                    printk(KERN_ERR __VA_ARGS__)
#endif

#define FASTBAT_WARN_ON(...)					\
do {											\
	if(__ratelimit(&fastbat_warn_state))		\
		WARN_ON(__VA_ARGS__);					\
} while(0)

#define FASTBAT_BUG_ON(...)						\
do {											\
	if(__ratelimit(&fastbat_bug_state))			\
		BUG_ON(__VA_ARGS__);					\
} while(0)

static DEFINE_MUTEX(bats_lock);
static DEFINE_MUTEX(local_lock);
static struct hlist_head	bats[FAST_BAT_LIST_LEN];	//record all bats
static u8	forbid_mac_addr[][ETH_ALEN+1] = {
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 6},	//"broadcast"
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 6},	//"zero addr"
	{0x01, 0x80, 0xc2, 0x00, 0x00, 0x00, 6},	//"stp addr": spanning tree
	{0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 6},	//"ectp addr": ethernet v2.0 configuration testing
	{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 1},	//"multicast": is_multicast_ether_addr(addr)
	{0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 2},	//multicast of ipv6
	{NULL}, 
};
struct batadv_priv  *bat_priv=NULL;

static int	fastbat_clone_header(void *src, void *dst);
static u8   fastbat_grab_priv_ttvn(struct net_device *);
static int  fastbat_xmit_wrap(struct sk_buff *, FAST_BAT *);
int  fastbat_xmit_peel(struct sk_buff *, FAST_LOCAL *);
static int  fastbat_xmit(struct sk_buff *, FAST_BAT *);
static int  fastbat_insert_list_entry(u8 *src, u8 *dst, FAST_BAT *);
static u32  fastbat_hash_calculate(u8 *src, u8 *dst);
static u8   fastbat_verdict_phyaddr(u8 *addr);
static u8   fastbat_verdict_packettype(struct ethhdr *);
static void	fastbat_show_list_entry(u32 idx);
static FAST_BAT	*fastbat_skb_peel_and_search(struct sk_buff *);
static void fastbat_del_list_entry(FAST_BAT *);
static struct	batadv_orig_node *fastbat_grab_orig_by_neigh(u8 *addr, struct net_device *); 
static void	fastbat_del_local_entry(FAST_LOCAL *);
static void	fastbat_show_local_entry(void);

/* sysctl variable to turn on/off fastbat dynamically */
extern int fastmesh_enable;
extern struct list_head fastbat_local_head;

/* input: src/dst mac
 *
 * return 0~(FAST_BAT_LIST_LEN-1)
 * return 0xfffffffe if failed
 * 
 * hash-1: src[0] src[1] src[2] src[3]
 * hash-2: dst[0] dst[1] dst[2] dst[3]
 * hash-3: dst[4] src[4] dst[5] src[5]
 */
static u32	fastbat_hash_calculate
(u8 *src, 
 u8 *dst)
{
	u32	a, b, c=0, hash=0;

	if((src==NULL) || (dst==NULL))
		return	0xfffffffe;

	a=*(src)<<24 | *(src+1)<<16 | *(src+2)<<8 | *(src+3);
	b=*(dst)<<24 | *(dst+1)<<16 | *(dst+2)<<8 | *(dst+3);
	c=*(dst+4)<<24 | *(src+4)<<16 | *(dst+5)<<8 | *(src+5);
	hash=jhash_3words(a, b, c, BATADV_COMPAT_VERSION);
	return	(hash%FAST_BAT_LIST_LEN);
}

/* Example:
 * u32 idx=fastbat_hash_calculate(src, dst);
 * if(idx == 0xfffffffe)
 *    //[F@STBAT] error hash value!
 * fastbat_insert_list_entry(src, dst, bat);
 * 
 * 
 * return 0 if successful
 *        -1 if failed
 * input: 
 * - src: source mac
 * - dst: dest mac
 */
static int	fastbat_insert_list_entry
(u8 *src, 
 u8 *dst, 
 FAST_BAT *bat)
{
	u32	idx=fastbat_hash_calculate(src, dst);

	if(bat == NULL)
		return	-EINVAL;
	if(idx >= FAST_BAT_LIST_LEN)
		return	-EINVAL;

	INIT_HLIST_NODE(&bat->list);
	mutex_lock(&bats_lock);
	hlist_add_head(&bat->list, &bats[idx]);
	mutex_unlock(&bats_lock);
	return	0;
}

#if (FASTBAT_DBG == 1)
/* dump all hash entry to debug
 *
 */
static void		fastbat_show_all_list_entry
(void)
{
	int			idx=0;
	FAST_BAT	*bat;

	FASTBAT_DBG_RAW_MSG("[F@STBAT] all entries:\n");
	FASTBAT_DBG_RAW_MSG("<src>  <dst>  <orig>  <dirty>  <soft>  <out>  <vlan>  <batman>  <batman-type>\n");
	for(; idx<FAST_BAT_LIST_LEN; idx++) {
		if(!hlist_empty(&bats[idx])) {
			FASTBAT_DBG_RAW_MSG("<idx-%d>\n", idx);
			hlist_for_each_entry(bat, &bats[idx], list) {
				FASTBAT_DBG_RAW_MSG("%x:%x:%x:%x:%x:%x  %x:%x:%x:%x:%x:%x  %x:%x:%x:%x:%x:%x  %d %s %s %d %d %d\n", 
					bat->src[0], bat->src[1], bat->src[2], 
					bat->src[3], bat->src[4], bat->src[5], 
					bat->dst[0], bat->dst[1], bat->dst[2], 
					bat->dst[3], bat->dst[4], bat->dst[5], 
					bat->orig[0], bat->orig[1], bat->orig[2], 
					bat->orig[3], bat->orig[4], bat->orig[5], 
					bat->dirty, bat->soft_iface->name, bat->net_dev->name, bat->vlan, bat->batman, bat->batman_type);
			}
		}
	}
}

static void		fastbat_show_list_entry
(u32 idx)
{
	FAST_BAT	*bat;
	if(hlist_empty(&bats[idx])) {
		FASTBAT_DBG_RAW_MSG("[F@STBAT-%d] is NULL!!!\n", idx);
		return;
	}
	FASTBAT_DBG_RAW_MSG("[F@STBAT-%d]\n", idx);
	hlist_for_each_entry(bat, &bats[idx], list) {
		FASTBAT_DBG_RAW_MSG("(src)%x:%x:%x:%x:%x:%x\n (dst)%x:%x:%x:%x:%x:%x\n (real src)%x:%x:%x:%x:%x:%x\n (real dst)%x:%x:%x:%x:%x:%x\n (orig)%x:%x:%x:%x:%x:%x\n (dirty)%d (soft)%s (out)%s (vlan)%d (batman)%d (batman-type)%d\n", 
			bat->src1[0], bat->src1[1], bat->src1[2], bat->src1[3], bat->src1[4], bat->src1[5],
			bat->dst1[0], bat->dst1[1], bat->dst1[2], bat->dst1[3], bat->dst1[4], bat->dst1[5],
			bat->src[0], bat->src[1], bat->src[2], bat->src[3], bat->src[4], bat->src[5], 
			bat->dst[0], bat->dst[1], bat->dst[2], bat->dst[3], bat->dst[4], bat->dst[5], 
			bat->orig[0], bat->orig[1], bat->orig[2], bat->orig[3], bat->orig[4], bat->orig[5], 
			bat->dirty, bat->soft_iface->name, bat->net_dev->name, bat->vlan, bat->batman, bat->batman_type);
	}
}

static void		fastbat_show_local_entry
(void)
{
	FAST_LOCAL	*local;
	FASTBAT_DBG_RAW_MSG("[F@STBAT-%d]: \n");
	list_for_each_entry(local, &fastbat_local_head, list) {
		FASTBAT_DBG_RAW_MSG("(dst)%pM (dirty)%d (out)%s\n", local->dst, local->dirty, local->output_dev->name);
		
	}
}

/* max input len should be 256
 */
void	fastbat_show_raw_data
(u8 *begin, 
 u8 len)
{
	u8	idx=0;
	for(idx=0 ;idx<len; idx+=16) {
		FASTBAT_DBG_RAW_MSG("[%x] %x %x %x %x %x %x %x %x   %x %x %x %x %x %x %x %x\n", 
			begin, *begin, *(begin+1), *(begin+2), *(begin+3), *(begin+4), *(begin+5), *(begin+6), *(begin+7), 
			*(begin+8), *(begin+9), *(begin+10), *(begin+11), *(begin+12), *(begin+13), *(begin+14), *(begin+15));
		begin += 16;
	}
}

#define xxxfastbat_show_raw_data(...)
#else
static void     fastbat_show_all_list_entry
(void)
{
	return;
}

static void     fastbat_show_list_entry
(u32 idx)
{
	return;
}

static void		fastbat_show_local_entry
(void)
{
	return;
}

void	fastbat_show_raw_data
(u8 *begin, 
 u8 len)
{
	return;
}

#define xxxfastbat_show_raw_data(...)
#endif

/* TODO: double vlan
 * 
 * peel outgoing skb to grab src and dst addr and search jhash index 
 */
static FAST_BAT		*fastbat_skb_peel_and_search
(struct sk_buff *skb)
{
	FAST_BAT	*bat;
	int			hdr_len=0;
	struct ethhdr		*ethhdr=eth_hdr(skb), 
						*real_ethhdr;
	struct vlan_ethhdr	*vhdr;
	struct batadv_unicast_packet	*u;
	struct batadv_unicast_4addr_packet	*u4;

	xxxfastbat_show_raw_data((u8 *)ethhdr, 48);
	if(ethhdr->h_proto == ETH_P_8021Q) {
		hdr_len=ETH_VLAN_HLEN;
		vhdr=vlan_eth_hdr(skb);
		if(vhdr->h_vlan_encapsulated_proto == ETH_P_BATMAN) {
			u=((struct batadv_unicast_packet *)(vhdr+1));
			if(u->packet_type == BATADV_UNICAST)
				hdr_len += sizeof(struct batadv_unicast_packet);
			else if(u->packet_type == BATADV_UNICAST_4ADDR)
				hdr_len += sizeof(struct batadv_unicast_4addr_packet);
			else
				goto	RETN;
		} else {
			FASTBAT_ERR_MSG(" Err! vlan but w/o batman header!\n");
			goto	RETN;
		}
	} else if(ethhdr->h_proto == ETH_P_BATMAN) {
		hdr_len=ETH_HLEN;
		u=((struct batadv_unicast_packet *)(ethhdr+1));
		if(u->packet_type == BATADV_UNICAST)
			 hdr_len += sizeof(struct batadv_unicast_packet);
		else if(u->packet_type == BATADV_UNICAST_4ADDR)
			hdr_len += sizeof(struct batadv_unicast_4addr_packet);
		else
			goto	RETN;
	}
	real_ethhdr=(struct ethhdr *)((u8 *)ethhdr+hdr_len);
	bat=fastbat_list_search(real_ethhdr->h_source, real_ethhdr->h_dest);
RETN:
	return	bat;
}

/* find out orig by neigh's addr 
 * return orig_node if found
 * return NULL if failed
 */
static struct batadv_orig_node	*fastbat_grab_orig_by_neigh
(u8 *addr, 
 struct net_device *soft)
{
	u32			idx=0;
	//struct batadv_priv	*bat_priv=netdev_priv(soft);
	struct hlist_head	*head;
	struct batadv_orig_node	*orig_node;
	struct batadv_neigh_node	*neigh_node;
	struct batadv_hashtable	*hash;//=bat_priv->orig_hash;

	if(bat_priv == NULL)
		bat_priv=netdev_priv(soft);
		
	hash=bat_priv->orig_hash;
	//if(!soft) {
	//	FASTBAT_DBG_RAW_MSG("[F@STBAT] Err! no soft iface!\n");
	//	return	NULL;
	//}
	//bat_priv=netdev_priv(soft);
	//hash=bat_priv->orig_hash;
	for(; idx<hash->size; idx++) {
		head=&hash->table[idx];
		hlist_for_each_entry(orig_node, head, hash_entry) {
			hlist_for_each_entry(neigh_node, &orig_node->neigh_list, list) {
				xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] orig is %pM and neigh is %pM\n", orig_node->orig, neigh_node->addr);
				if(!memcmp(addr, neigh_node->addr, ETH_ALEN))
					return	orig_node;
			}
		}
	}
	FASTBAT_ERR_MSG("[F@STBAT] failed to find orig!\n");
	return	NULL;
}

/* check if addr is one tt local host
 * return 1 if it is a local host
 * return 0 when it is not a local host
 */
static int	fastbat_check_local_tt
(u8 *addr)
{
	int	retval=0, 
		idx=0;
	struct batadv_tt_common_entry	*tt_common;
	struct batadv_tt_local_entry	*tt_local;
	struct hlist_head				*head;
	
	if((bat_priv==NULL) || (bat_priv->tt.local_hash==NULL))
		goto	RETN;
	for(; idx<bat_priv->tt.local_hash->size; idx++) {
		head=&bat_priv->tt.local_hash->table[idx];
		rcu_read_lock();
		hlist_for_each_entry(tt_common, head, hash_entry) {
			xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] addr:%pM %d\n", 
				tt_common->addr, batadv_compare_eth(tt_common, addr));
			if(retval=batadv_compare_eth(tt_common, addr)) {
				goto	RETN;
			}
		}
		rcu_read_unlock();
	}
RETN:
	return	retval;
}

/* TODO: vlan
 * return 0 if successful
 */
int		fastbat_chk_and_add_local_entry
(struct sk_buff *skb, 
 struct net_device *out)
{
	FAST_LOCAL	*local;
	int			retval= -EINVAL;
	struct ethhdr	*ethhdr=eth_hdr(skb);

	/* checking */
	/*if(out == NULL) {
		FASTBAT_ERR_MSG(" output dev is NULL!!!\n");
		goto	RETN;
	}*/
	if(!fastbat_verdict_phyaddr(ethhdr->h_dest)) {
		xxxFASTBAT_DBG_MSG(" prohibit PHY addr!!!\n");
		goto	RETN;
	}
	if(!memcmp(out->name, "bat", 3)) {
		xxxFASTBAT_DBG_MSG(" prohibit bat iface!!!\n");
		goto    RETN;
	}
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] %pM %s\n", ethhdr->h_dest, out->name);

	/* search */
	local=fastbat_local_search(ethhdr->h_dest);
	if(local) {
		xxxFASTBAT_DBG_MSG("[F@STBAT] local bat's exist!\n");
		retval=-EEXIST;
		goto	RETN;
	}
	/* recheck if addr is exist in local tt host to avoid wrong insert */
	/* check local transtable */
	if(!fastbat_check_local_tt(ethhdr->h_dest)) {
		xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] non-exist in local tt - %pM \n", ethhdr->h_dest);
		retval=-EINVAL;
		goto	RETN;
	}

	/* append */
	local=kzalloc(sizeof(FAST_LOCAL), GFP_KERNEL);
	if(local == NULL) {
		retval=-ENOMEM;
		goto	RETN;
	}
	memcpy(local->dst, ethhdr->h_dest, ETH_ALEN);
	local->last_seen=jiffies;
	local->output_dev=out;
	list_add(local, &fastbat_local_head);

RETN:
	return		retval;
}

/* TODO: batadv_unicast_4addr_packet part and double vlan part
 * input neigh: can be null
 * return 0 if successful
 */
int		fastbat_chk_and_add_list_entry
(struct sk_buff *skb, 
 struct net_device *soft, 
 struct net_device *out, 
 u8 *neigh)
{
	FAST_BAT		*bat;
	int				retval=-EINVAL, 
					hdr_len=0;
	bool			is4addr;
	u8				packettype=0;
	struct ethhdr	*ethhdr=eth_hdr(skb), 
					*real_ethhdr;
	struct vlan_ethhdr	*vhdr;
	struct batadv_unicast_packet		*unicast, 
										*u;
	struct batadv_unicast_4addr_packet	*unicast_4addr, 
										*u4;

	/* chking */
	if(!fastbat_verdict_phyaddr(ethhdr->h_dest)) {
		xxxFASTBAT_DBG_MSG(" prohibit PHY addr!!!\n");
		//retval=-EINVAL;
		goto	RETN;
	}
	if((soft==NULL) || (out==NULL)) {
		FASTBAT_ERR_MSG(" soft/out dev is NULL!!!\n");
		retval=-ENODEV;
		goto    RETN;
	}
#if	0//debug
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] (proto=%x) (DST=%x:%x:%x:%x:%x:%x) (SRC=%x:%x:%x:%x:%x:%x)\n", 
		ethhdr->h_proto, 
		ethhdr->h_dest[0], ethhdr->h_dest[1], ethhdr->h_dest[2], 
		ethhdr->h_dest[3], ethhdr->h_dest[4], ethhdr->h_dest[5], 
		ethhdr->h_source[0], ethhdr->h_source[1], ethhdr->h_source[2], 
		ethhdr->h_source[3], ethhdr->h_source[4], ethhdr->h_source[5]);
	xxxfastbat_show_raw_data((u8 *)ethhdr, 63);
#endif

HDR_PARSE:
	/*if(!fastbat_verdict_packettype(ethhdr)) {
		FASTBAT_DBG_RAW_MSG("[F@STBAT] PACKET TYPE verdict failed!!!");
		retval=-EINVAL;
		goto    RETN;
	}*/
	if(ethhdr->h_proto == ETH_P_8021Q) {
		hdr_len=VLAN_ETH_HLEN;
		vhdr=vlan_eth_hdr(skb);
		if(vhdr->h_vlan_encapsulated_proto == ETH_P_BATMAN) {
			u=((struct batadv_unicast_packet *)(vhdr+1));
			packettype=u->packet_type;
			if(u->packet_type == BATADV_UNICAST)
				hdr_len += sizeof(struct batadv_unicast_packet);
			/*else if(u->packet_type == BATADV_UNICAST_4ADDR) {
				//hdr_len += sizeof(struct batadv_unicast_4addr_packet);
				FASTBAT_DBG_RAW_MSG("[F@STBAT] Warning! 4addr!\n");
				retval=-EINVAL;
				goto	RETN;
			} */else {
				retval=-EINVAL;
				goto	RETN;
			}
		} else {
			FASTBAT_ERR_MSG("[F@STBAT] Err! vlan but w/o batman header!\n");
			retval=-EINVAL;
			goto	RETN;
		}
	} else if(ethhdr->h_proto == ETH_P_BATMAN) {
		hdr_len=ETH_HLEN;
		u=((struct batadv_unicast_packet *)(ethhdr+1));
		packettype=u->packet_type;
		if(u->packet_type == BATADV_UNICAST)
			hdr_len += sizeof(struct batadv_unicast_packet);
		/*else if(u->packet_type == BATADV_UNICAST_4ADDR) {
			//hdr_len += sizeof(struct batadv_unicast_4addr_packet);
			FASTBAT_DBG_RAW_MSG("[F@STBAT] Warning! 4addr!\n");
			retval=-EINVAL;
			goto	RETN;
		} */else {
			retval=-EINVAL;
			goto	RETN;
		}
	}
	real_ethhdr=(struct ethhdr *)((u8 *)ethhdr+hdr_len);
#if	0//debug
	FASTBAT_DBG_RAW_MSG("[F@STBAT] proto(%x)\n", real_ethhdr->h_proto);
	FASTBAT_DBG_RAW_MSG("[F@STBAT] (real) (ptype=%x) (proto=%x) (DST=%x:%x:%x:%x:%x:%x) (SRC=%x:%x:%x:%x:%x:%x)\n", 
		packettype, real_ethhdr->h_proto, 
		real_ethhdr->h_dest[0], real_ethhdr->h_dest[1], real_ethhdr->h_dest[2], 
		real_ethhdr->h_dest[3], real_ethhdr->h_dest[4], real_ethhdr->h_dest[5], 
		real_ethhdr->h_source[0], real_ethhdr->h_source[1], real_ethhdr->h_source[2], 
		real_ethhdr->h_source[3], real_ethhdr->h_source[4], real_ethhdr->h_source[5]);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] (IDX=%u) (hdr-len%d)\n", 
		fastbat_hash_calculate(real_ethhdr->h_source, real_ethhdr->h_dest), hdr_len);
#endif

	//bat=fastbat_skb_peel_and_search(skb);
	//fastbat_show_raw_data((u8 *)ethhdr, 48);
	bat=fastbat_list_search(real_ethhdr->h_source, real_ethhdr->h_dest);
	if(bat) {
		//fastbat_show_list_entry(fastbat_hash_calculate(bat->src, bat->dst));
		//FASTBAT_DBG_MSG(" SRC/DST's exist!!!");
		retval=-EEXIST;
		goto	RETN;
	}
	bat=kzalloc(sizeof(FAST_BAT), GFP_KERNEL);
	if(bat == NULL) {
		FASTBAT_BUG_ON(1);
		retval=-ENOMEM;
		goto	RETN;
	}

	/* append into list */
	ether_addr_copy(bat->src1, ethhdr->h_source);
	ether_addr_copy(bat->dst1, ethhdr->h_dest);
	if(ethhdr->h_proto == ETH_P_8021Q) {
		bat->vlan=1;
		hdr_len=VLAN_ETH_HLEN;
		vhdr=vlan_eth_hdr(skb);
		bat->vlan_data.h_vlan_TCI=vhdr->h_vlan_TCI;
		bat->vlan_data.h_vlan_encapsulated_proto=vhdr->h_vlan_encapsulated_proto;
		if(vhdr->h_vlan_encapsulated_proto == ETH_P_BATMAN) {
			bat->batman=1;
DISASSEMBLE:
			unicast=(struct batadv_unicast_packet *)((u8 *)vhdr+hdr_len);
			if(unicast->packet_type == BATADV_UNICAST) {
				hdr_len += sizeof(struct batadv_unicast_packet);
				bat->bat_data.u.packet_type=unicast->packet_type;
				bat->bat_data.u.version=unicast->version;
				bat->bat_data.u.ttl=unicast->ttl;
				bat->bat_data.u.ttvn=unicast->ttvn;
				bat->batman_type=BATADV_UNICAST;
				ether_addr_copy(bat->bat_data.u.dest, unicast->dest);
			/*} else if(unicast->packet_type == BATADV_UNICAST_4ADDR) {
				hdr_len += sizeof(struct batadv_unicast_4addr_packet);
				unicast_4addr=(struct batadv_unicast_4addr_packet *)unicast;
				bat->bat_data.u4.u.packet_type=unicast_4addr->u.packet_type;
				bat->bat_data.u4.u.version=unicast_4addr->u.version;
				bat->bat_data.u4.u.ttl=unicast_4addr->u.ttl;
				bat->bat_data.u4.u.ttvn=unicast_4addr->u.ttvn;
				bat->batman_type=BATADV_UNICAST_4ADDR;
				ether_addr_copy(bat->bat_data.u4.u.dest, unicast_4addr->u.dest);
				ether_addr_copy(bat->bat_data.u4.src, unicast_4addr->src);
				bat->bat_data.u4.subtype=unicast_4addr->subtype;
				bat->bat_data.u4.reserved=unicast_4addr->reserved;*/
			} else if(unicast->packet_type == ETH_P_8021Q) {
				FASTBAT_DBG_RAW_MSG(" warning! vlan in vlan!\n");
				hdr_len += VLAN_HLEN;
				goto	DISASSEMBLE;
			} else {
				FASTBAT_DBG_RAW_MSG("[FASTBAT] !BATADV_UNICAST & !BATADV_UNICAST_4ADDR but (%x)\n", 
					unicast->packet_type);
				goto	FREE;
			}
		} else {
			FASTBAT_ERR_MSG(" Error! No batman header!\n");
			goto	FREE;
		}
	} else if(ethhdr->h_proto == ETH_P_BATMAN) {
		bat->batman=1;
		hdr_len=ETH_HLEN;
		unicast=(struct batadv_unicast_packet *)(ethhdr+1);
		if(unicast->packet_type == BATADV_UNICAST) {
			hdr_len += sizeof(struct batadv_unicast_packet);
			bat->bat_data.u.packet_type=BATADV_UNICAST;//unicast->packet_type;
			bat->bat_data.u.version=unicast->version;
			bat->bat_data.u.ttl=unicast->ttl;
			bat->bat_data.u.ttvn=unicast->ttvn;
			bat->batman_type=BATADV_UNICAST;
			ether_addr_copy(bat->bat_data.u.dest, unicast->dest);
		/*} else if(unicast->packet_type == BATADV_UNICAST_4ADDR) {
			hdr_len += sizeof(struct batadv_unicast_4addr_packet);
			unicast_4addr=(struct batadv_unicast_4addr_packet *)unicast;
			bat->bat_data.u4.u.packet_type=BATADV_UNICAST_4ADDR;//unicast_4addr->u.packet_type;
			bat->bat_data.u4.u.version=unicast_4addr->u.version;
			bat->bat_data.u4.u.ttl=unicast_4addr->u.ttl;
			bat->bat_data.u4.u.ttvn=unicast_4addr->u.ttvn;
			bat->batman_type=BATADV_UNICAST_4ADDR;
			ether_addr_copy(bat->bat_data.u4.u.dest, unicast_4addr->u.dest);
			ether_addr_copy(bat->bat_data.u4.src, unicast_4addr->src);
			bat->bat_data.u4.subtype=unicast_4addr->subtype;
			bat->bat_data.u4.reserved=unicast_4addr->reserved;*/
		} else {
			FASTBAT_DBG_RAW_MSG("[F@STBAT] !BATADV_UNICAST & !BATADV_UNICAST_4ADDR but (%x)\n", 
				unicast->packet_type);
			goto	FREE;
		}
	}
	ethhdr=(struct ethhdr *)((u8 *)ethhdr + hdr_len);
	bat->net_dev=out;
	bat->soft_iface=soft;
	retval=fastbat_insert_list_entry(ethhdr->h_source, ethhdr->h_dest, bat);
	if(retval < 0) {
		retval=-EINVAL;
		goto	FREE;
	}
	bat->last_seen=jiffies;
	//bat->dirty=0;
	//bat->count=0;
	ether_addr_copy(bat->src, ethhdr->h_source);
	ether_addr_copy(bat->dst, ethhdr->h_dest);
	if(neigh) {
		struct batadv_orig_node	*orig_node;
		orig_node=fastbat_grab_orig_by_neigh(neigh, soft);
		if(orig_node != NULL)
			ether_addr_copy(bat->orig, orig_node->orig);
	}

	FASTBAT_DBG_RAW_MSG("[F@STBAT] insert new list!\n");
	fastbat_show_list_entry(fastbat_hash_calculate(bat->src, bat->dst));
	return	0;
FREE:
	kfree(bat);
RETN:
	return	retval;
}
EXPORT_SYMBOL(fastbat_chk_and_add_list_entry);

static void		fastbat_del_local_entry
(FAST_LOCAL *local)
{
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] dirty local bat! eliminates this bat!\n");
	mutex_lock(&local_lock);
	list_del(local);
	mutex_unlock(&local_lock);
	return;
}

static void		fastbat_del_list_entry
(FAST_BAT *bat)
{
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] dirty bat! eliminates this bat!\n");
	mutex_lock(&bats_lock);
	__hlist_del(&bat->list);
	mutex_unlock(&bats_lock);
	kfree(bat);
}

/* return NULL if non-exist
 * return corresponding FAST_LOCAL if found
 */
FAST_LOCAL	*fastbat_local_search
(u8 *addr)
{
	FAST_LOCAL	*local;
	list_for_each_entry(local, &fastbat_local_head, list) {
		if(memcmp(local->dst, addr, ETH_ALEN) == 0) {
			if(!local->dirty) {
				return	local;
			} else {
				fastbat_del_local_entry(local);
				fastbat_show_local_entry();
				break;
			}
		}
	}
	return	NULL;
}
EXPORT_SYMBOL(fastbat_local_search);

/* return NULL if failed
 * return corresponding FAST_BAT ptr if exist
 */
FAST_BAT	*fastbat_list_search
(u8 *src, 
 u8 *dst)
{
	u32			idx=0;
	int			retval=0;
	FAST_BAT	*bat;

	/* data reading don't really need mutex to protect */
	idx=fastbat_hash_calculate(src, dst);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] INPUT(idx=%d) SRC:%x:%x:%x:%x:%x:%x DST:%x:%x:%x:%x:%x:%x\n", 
		idx, src[0], src[1], src[2], src[3], src[4], src[5], 
		dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
	if(idx == 0xfffffffe)
		goto	RETN;
	hlist_for_each_entry(bat, &bats[idx], list) {
		xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] ENTRY SRC:%x:%x:%x:%x:%x:%x DST:%x:%x:%x:%x:%x:%x\n", 
			bat->src[0], bat->src[1], bat->src[2], bat->src[3], bat->src[4], bat->src[5], 
			bat->dst[0], bat->dst[1], bat->dst[2], bat->dst[3], bat->dst[4], bat->dst[5]);
		/* src and dst addr are both the same*/
		if((memcmp(bat->src, src, ETH_ALEN)==0) && (memcmp(bat->dst, dst, ETH_ALEN)==0)){
			xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] Catch bat!!!\n");
			if(!bat->dirty) {
				return	bat;
			} else {
				fastbat_del_list_entry(bat);
				fastbat_show_list_entry(idx);
				goto	RETN;
			}
		}
	}
RETN:
	return	NULL;
}
EXPORT_SYMBOL(fastbat_list_search);

/* input skb and verdict if ETH_P_BATMAN or else
 * return ETH_P_BATMAN or else
 */
static inline u16	fastbat_grab_prototype
(struct sk_buff *skb)
{
	return  eth_hdr(skb)->h_proto;
}

/* TODO: double vlan
 * input : vlan => 0/1
 * return packet_type of batman (ELP/OGM/BCAST/UNICAST/UNICAST_4ADDR/...)
 */
static inline u8	fastbat_grab_batman_packettype
(struct ethhdr *ethhdr, 
 int vlan)
{
	u8	*packet_type;

	if(!vlan) {
		packet_type=(u8 *)ethhdr + ETH_HLEN;
	} else /*if(valn == 1)*/{
		packet_type=(u8 *)ethhdr + VLAN_ETH_HLEN;
	}/* else if (vlan == 2) {
		packet_type=(u8 *)ethhdr + VLAN_ETH_HLEN + VLAN_HLEN;
	} */
		
	return	*packet_type;
}

#if	0
/* TODO: double vlan
 * input : vlan => 0/1
 * dont handle these packet and handle by upper layer, return 0
 * return 1(batman's unicast packet), deliver directly
 */
static u8	fastbat_verdict_packettype
(struct ethhdr *ethhdr)
{
	u8	packet_type;
	u8	pass=0;	
	u8	vlan=0;

	if(ethhdr->h_proto == ETH_P_BATMAN) {
		vlan=0;
		//return	1;
	} else if(ethhdr->h_proto == ETH_P_8021Q) {
		vlan=1;
	} else {
		vlan=0;
	}
	/* else {
		FASTBAT_DBG_RAW_MSG("[F@STBAT] proto(%x)\n", ethhdr->h_proto);
		return	1;			//???
		//packet_type=ethhdr->h_proto;
		//goto	CHK;
	}*/
	packet_type=fastbat_grab_batman_packettype(ethhdr, vlan);
	//if((packet_type != BATADV_ELP) && (packet_type != BATADV_OGM2) && (packet_type != BATADV_IV_OGM) && (packet_type != BATADV_BCAST))
//CHK:
	if((packet_type==BATADV_UNICAST_4ADDR) || (packet_type==BATADV_UNICAST))
		pass=1;
	return	pass;
}
#endif

/* dont handle these packets here and pass to upper layer, return 0
 * return 1, deliver directly
 */
static u8	fastbat_verdict_phyaddr
(u8 *addr)
{
	u8	pass=1;
	u8	idx=0;

	/*if(addr == NULL) {
		FASTBAT_DBG_MSG(" Err! Null ptr!\n");
		pass=0;
		goto	RETN;
	}*/
	for(; forbid_mac_addr[idx][6] != NULL; idx++) {
		if(memcmp(&forbid_mac_addr[idx][0], addr, forbid_mac_addr[idx][6]) == 0) {
			pass=0;
			break;
		}
	}
RETN:
	return	pass;
}

#if	0
/* if no batman header, pack it and deliver
 * if there is batman header, pass it to next orig directly
 */
int		fastbat_verdict_header
(struct sk_buff *skb)
{
	struct ethhdr	*ethhdr;
	u16	proto;
	//int	pass=0;
	//u8	packettype;

	//ethhdr=eth_hdr(skb);
	//proto=ethhdr->h_proto;
	proto=fastbat_grab_prototype(skb);
	//proto=eth_type_trans(skb, input_dev);
	pass=fastbat_check_packettype(ethhdr)
	if(pass)
		return	1;
	else
		
	return	0;
}
#endif

/* return NULL if not BATADV_UNICAST/BATADV_UNICAST_4ADDR
 * return 1st-layer ethernet header if not ETH_P_8021Q/ETH_P_BATMAN
 * return 2nd-layer ethernet header if ETH_P_8021Q/ETH_P_BATMAN
 */
struct ethhdr	*fastbat_grab_real_ethhdr
(struct sk_buff *skb)
{
	struct ethhdr		*eth, 
						*real_eth;
	struct vlan_ethhdr	*vhdr;
	int					hdr_len=0;
	struct batadv_unicast_packet		*u;
	//struct batadv_unicast_4addr_packet	*u4;

	eth=eth_hdr(skb);
	if(eth->h_proto == ETH_P_BATMAN) {
		//hdr_len=ETH_HLEN;
		u=(struct batadv_unicast_packet *)((u8 *)eth + ETH_HLEN);
		if(u->packet_type == BATADV_UNICAST)
			hdr_len=sizeof(struct batadv_unicast_packet);
		else if(u->packet_type == BATADV_UNICAST_4ADDR)
			hdr_len=sizeof(struct batadv_unicast_4addr_packet);
		else {
			FASTBAT_DBG_MSG(" warning! other types of packets!\n");
			return	NULL;
		}
		real_eth=(struct ethhdr *)((u8 *)u + hdr_len);
	} else if(eth->h_proto == ETH_P_8021Q) {			//ETH_P_8021Q
		//hdr_len=VLAN_ETH_HLEN;
		vhdr=vlan_eth_hdr(skb);
		if(vhdr->h_vlan_encapsulated_proto != ETH_P_BATMAN) {
			return	eth;
		}
		u=(struct batadv_unicast_packet *)((u8 *)vhdr + VLAN_ETH_HLEN);
		if(u->packet_type == BATADV_UNICAST)
			hdr_len=sizeof(struct batadv_unicast_packet);
		else if(u->packet_type == BATADV_UNICAST_4ADDR)
			hdr_len=sizeof(struct batadv_unicast_4addr_packet);
		else {
			FASTBAT_DBG_MSG(" warning! other type packets!\n");
			return	NULL;
		}
	} else {
		return  eth;
	}
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] protocol:0x%x len:%d\n", eth->h_proto, hdr_len);
	real_eth=(struct ethher *)((u8 *)u + hdr_len);
	return	real_eth;
}

/* TODO: double vlan
 * 1st step: filter ARP/...
 * 2nd step: filter elp/ogm/... packet
 * 3rd step: filter forbid_mac_addr
 * return 1 => xmit directly if bat's exist
 */
int		fastbat_filter_input_packet
(struct sk_buff *skb, 
 u16 *vlan, 
 u16 *batman)
{
	int		pass=0;
	struct	vlan_ethhdr	*vhdr;
	struct	ethhdr		*ethhdr=eth_hdr(skb);

	*vlan=0;
	*batman=0;
#if	1//debug
	xxxfastbat_show_raw_data((u8 *)ethhdr, 63);
#endif
	/* verdict proto type of ethernet */
	if((ethhdr->h_proto==ETH_P_ARP) || (ethhdr->h_proto==0x893a)) {
		goto	RETN;
	} else if(ethhdr->h_proto == ETH_P_BATMAN) {
		*batman=1;
		pass=fastbat_grab_batman_packettype(ethhdr, *vlan);
	} else if(ethhdr->h_proto == ETH_P_8021Q) {
		*vlan=1;
		vhdr=vlan_eth_hdr(skb);
		if(vhdr->h_vlan_encapsulated_proto == ETH_P_BATMAN) {
			*batman=1;
			pass=fastbat_grab_batman_packettype(ethhdr, *vlan);
		} /*else if(vhdr->h_vlan_encapsulated_proto == ETH_P_8021Q) {
			
		} */else {
			FASTBAT_DBG_MSG(" Err! No batman header!\n");
		}
	}
	if(pass)
		goto	RETN;

	/* verdict mac addr */
	pass=fastbat_verdict_phyaddr(ethhdr->h_dest);	//dont filter real dest mac???
RETN:
#if	0//debug
	//if(pass)
		FASTBAT_DBG_RAW_MSG("[F@STBAT] (pass=%d/%04x) (vlan=%d) (batman=%d)\n", 
			pass, ethhdr->h_proto, *vlan, *batman);
#endif
#if	0//debug
	if(pass)
		fastbat_show_raw_data((u8 *)ethhdr, 63);
#endif
	return	pass;
}
EXPORT_SYMBOL(fastbat_filter_input_packet);

/* EX. 
 * pass=fastbat_filter_input_packet(skb, &vlan, &batman);
 * if(pass)
 *    bat=fastbat_list_search(ethhdr->h_source, ethhdr->dest);
 *    if(bat != NULL)
 *       fastbat_dispatch(skb, bat, vlan, batman);
 *
 * invoke fastbat_dispatch if find out corresponding bat
 * 
 * 1. vlan + batman
 * 2. batman
 * 3. !vlan + !batman
 *
 * return 0 if successful
 */
int		fastbat_dispatch
(struct sk_buff *skb, 
 FAST_BAT *bat, 
 u16 vlan, 
 u16 batman)
{
	int	retval=-1;

	if(bat == NULL)
		goto	RETN;
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] dispatch: vlan(%d)batman(%d) => vlan(%d)batman(%d)\n", 
		vlan, batman, bat->vlan, bat->batman);
	if(vlan && batman && bat->vlan && bat->batman)
		retval=fastbat_xmit(skb, bat);
	else if(!vlan && !batman && bat->vlan && bat->batman)
		retval=fastbat_xmit_wrap(skb, bat);
	else if(!vlan && batman && !bat->vlan && bat->batman)
		retval=fastbat_xmit(skb, bat);
	else if(!vlan && !batman && !bat->vlan && bat->batman)
		retval=fastbat_xmit_wrap(skb, bat);
	/*else if(vlan && batman && !bat->vlan && !bat->batman)
		retval=fastbat_xmit_peel(skb, bat);
	else if(!vlan && batman && !bat->vlan && !bat->batman)
		retval=fastbat_xmit_peel(skb, bat);*/
	else if(!vlan && !batman && !bat->vlan && !bat->batman)
		retval=fastbat_xmit(skb, bat);
	else
		FASTBAT_DBG_MSG(" Dispatch failed!\n");

#if	0//(FASTBAT_DBG==1)
	if(retval < 0)
		FASTBAT_DBG_RAW_MSG("[F@STBAT] fastbat_dispatch failed!(%d)\n", retval);
	else
		FASTBAT_DBG_RAW_MSG("[F@STBAT] fastbat_dispatch !\n");
#endif
RETN:
	return	retval;
}
EXPORT_SYMBOL(fastbat_dispatch);

#if	0
/* vlan???
 */
int		fastbat_wrap_header
(struct sk_buff *skb, 
 FAST_BAT *bat)
{
	u8	*buffer;
	buffer=kzalloc(skb->, GFP_KERNEL);
	if(buffer == NULL)
		return	-ENOMEM;
	return	0;
}
#endif

/* return error code if failed
 * return 0 if successful
 */
int		fastbat_xmit_check
(FAST_BAT *bat)
{
	int	retval=0;

	/* checking input */
	/*if(unlikely(bat->net_dev)) {
		retval=-ENODEV;
		goto    RETN;
	}*/
	/*if(unlikely(bat->soft_iface)) {
	if(bat->soft_iface != 
		retval=-ENODEV;
		goto    RETN;
	}*/
	if(!(bat->net_dev->flags & IFF_UP)) {
		retval=-EINVAL;
		goto	RETN;
	}
	if(bat->dirty == 1) {
		FASTBAT_DBG_MSG("[F@STBAT] ERR! wrong bat!\n");
		/* eliminate this bat */
		
		retval=-EINVAL;
	}
RETN:
	return	retval;
}

/* TODO: double vlan
 * wrap skb in vlan/batman header and then xmit
 */
static int	fastbat_xmit_wrap
(struct sk_buff *skb, 
 FAST_BAT *bat)
{
	int		retval=0;
	int		hdr_len=0;
	struct vlan_ethhdr	*vhdr;
	struct ethhdr		*ethhdr;

	retval=fastbat_xmit_check(bat);
	if(retval < 0)
		goto	ERR;
	/*if(!bat->batman) {
		retval=-EEXIST;
		FASTBAT_ERR_MSG(" Err! No batman header!\n");
		goto	ERR;
	}*/
	if(bat->batman) {
		if(bat->batman_type == BATADV_UNICAST) {
			hdr_len=sizeof(struct batadv_unicast_packet);
		} else if(bat->batman_type == BATADV_UNICAST_4ADDR) {
			hdr_len=sizeof(struct batadv_unicast_4addr_packet);
		} else {
			retval=-EINVAL;
			goto	ERR;
		}
	} else {
		retval=-EINVAL;
		FASTBAT_ERR_MSG(" Err! no batman header!\n");
		goto	ERR;
	}
	if(bat->vlan) {
		hdr_len += VLAN_ETH_HLEN; 
	} else {
		hdr_len += ETH_HLEN;
	}

	//move forward data ptr for hdr_len bytes, and reset mac header
	xxxfastbat_show_raw_data((void *)skb->data, 32);
	ethhdr=eth_hdr(skb);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] push ethhdr(%p) data(%p) head(%p) mac(%p)\n", 
		ethhdr, skb->data, skb->head, skb_mac_header(skb));

#if	(PUSH_THE_PACE == 0)
	/* if not the same, alter skb's data ptr location to real ethernet header */
	if((skb->data - skb->mac_header) != 0)
		skb_push(skb, (skb->data - skb->mac_header));
	/* keep alter data's ptr to ethernet header */
	retval=batadv_skb_head_push(skb, hdr_len);
	if(retval < 0) {
		FASTBAT_ERR_MSG(" batadv_skb_head_push failed!\n");
		retval=-ENOMEM;
		goto	ERR;
	}
#else
	//if((skb->data - skb->mac_header) != 0) {
		skb_push(skb, (skb->data - skb->mac_header) + hdr_len);
	//} else {
	//	skb_push(skb, hdr_len);
	//}
#endif
	skb_reset_mac_header(skb);
	//skb_set_network_header(skb, ETH_HLEN);

	if(bat->vlan) {
		vhdr=vlan_eth_hdr(skb);
		vhdr->h_vlan_TCI=bat->vlan_data.h_vlan_TCI;
		vhdr->h_vlan_encapsulated_proto=bat->vlan_data.h_vlan_encapsulated_proto;
		ether_addr_copy(vhdr->h_source, bat->src1);
		ether_addr_copy(vhdr->h_dest, bat->dst1);
		vhdr->h_vlan_proto=ETH_P_8021Q;
		xxxfastbat_show_raw_data((void *)skb->data, 32);
		retval=fastbat_clone_header((void *)bat, (void *)((struct vlan_ethhdr *)vhdr+1));
	} else {
		ethhdr=eth_hdr(skb);
		ether_addr_copy(ethhdr->h_dest, bat->dst1);
		ether_addr_copy(ethhdr->h_source, bat->src1);
		ethhdr->h_proto=ETH_P_BATMAN;
		xxxfastbat_show_raw_data((void *)skb->data, 32);
		retval=fastbat_clone_header((void *)bat, (void *)((struct ethhdr *)ethhdr+1));
	}
	if(retval < 0) {
		retval=-EINVAL;
		goto	ERR;
	}
	skb->dev=bat->net_dev;
	skb->protocol=htons(ETH_P_BATMAN);
#if	1
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] wrap xmit ethhdr(%p) data(%p) head(%p) mac(%p)\n", 
		ethhdr, skb->data, skb->head, skb_mac_header(skb));
	//fastbat_show_raw_data((void *)skb->data, 63);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] wrap xmit (s)%pM (d)%pM\n", ethhdr->h_source, ethhdr->h_dest);
	xxxFASTBAT_DBG_MSG("[F@STBAT] wrap xmit(%s VS %s)!\n", skb->dev->name, bat->net_dev->name);
#endif
	retval=dev_queue_xmit(skb);
	bat->last_seen=jiffies;			//update last_seen jiffies

	return	net_xmit_eval(retval);
ERR:
	FASTBAT_ERR_MSG(" wrap xmit failed code(%d)\n", retval);
	return	retval;
}

/* TODO: double vlan
 * 
 * take off header of vlan/batman and then xmit skb
 */
int		fastbat_xmit_peel
(struct sk_buff *skb, 
 FAST_LOCAL *local)
{
	int		retval=-EINVAL;
	struct ethhdr	*ethhdr;
	int		vlan=0;		/* if vlan is on or not */
	int		hdr_len=0, 
			bat_len=0;
	u16		h_proto=0;
	struct batadv_unicast_packet	*u;

	/* check */
	if(!local->output_dev->flags & IFF_UP)
		goto	ERR;
	ethhdr=eth_hdr(skb);
	if(ethhdr->h_proto == ETH_P_BATMAN) {
		hdr_len=ETH_HLEN;
	} else if(ethhdr->h_proto == ETH_P_8021Q) {
		hdr_len=ETH_VLAN_HLEN;
	} else {
		FASTBAT_ERR_MSG(" this protocol type isn't accepted!\n");
		goto	ERR;
	}
	
	u=(struct batadv_unicast_packet *)((u8 *)ethhdr + hdr_len);
	if(u->packet_type == BATADV_UNICAST) {
		bat_len=sizeof(struct batadv_unicast_packet);
	} else if(u->packet_type == BATADV_UNICAST_4ADDR) {
		bat_len=sizeof(struct batadv_unicast_4addr_packet);
	} else {
		FASTBAT_ERR_MSG(" this packet type isn't accepted!\n");
		goto	ERR;
	}
	ethhdr=(struct ethhdr *)((u8 *)u + bat_len);
	/* alter data ptr to real ethernet header */
	skb_pull_rcsum(skb, bat_len);
	skb_reset_mac_header(skb);

	skb->dev=local->output_dev;
	skb->protocol=htons(ethhdr->h_proto);
	retval=dev_queue_xmit(skb);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] peel xmit(%d)!\n", retval);
	return	retval;

ERR:
	xxxFASTBAT_DBG_MSG(" Xmit of peel failed! Back to batman %p %p\n", skb->data, skb->mac_header);
	return	retval;
}

/* copy from bat->bat_data to skb
 * src: FAST_BAT *
 * dst: struct batadv_unicast_packet *
 */
static int	fastbat_clone_header
(void *src, 
 void *dst)
{
	struct batadv_unicast_packet		*u;
	struct batadv_unicast_4addr_packet	*u4;
	FAST_BAT	*bat=src;
	u8			priv_ttvn;
	int			retval=0;

	/* if soft_iface is null, cannot alter packet's ttvn */
	if(bat->soft_iface == NULL) {
		FASTBAT_ERR_MSG(" Err! Null soft_iface!\n");
		retval=-EEXIST;
		return	retval;
	}
	u=(struct batadv_unicast_packet *)dst;
	/*if(u->packet_type && (bat->batman_type!=u->packet_type)) {
		FASTBAT_DBG_RAW_MSG(" Err! conflict packet type(%d VS %d)!\n", 
			u->packet_type, bat->batman_type);
		retval=-EINVAL;
		return	retval;
	}*/
	priv_ttvn=fastbat_grab_priv_ttvn(bat->soft_iface);
	xxxFASTBAT_DBG_MSG(" TTVN=%d\n", priv_ttvn);
	if(bat->batman_type == BATADV_UNICAST) {
		u->packet_type=BATADV_UNICAST;
		u->version=bat->bat_data.u.version;
		--u->ttl;
		u->ttvn=priv_ttvn;
		ether_addr_copy(u->dest, bat->bat_data.u.dest);
	} else if(bat->batman_type == BATADV_UNICAST_4ADDR) {
		u4=(struct batadv_unicast_4addr_packet *)u;
		u4->u.packet_type=BATADV_UNICAST_4ADDR;
		u4->u.version=bat->bat_data.u4.u.version;
		--u4->u.ttl;
		u4->u.ttvn=priv_ttvn;
		ether_addr_copy(u4->u.dest, bat->bat_data.u4.u.dest);
		ether_addr_copy(u4->src, bat->bat_data.u4.src);
		u4->subtype=bat->bat_data.u4.subtype;
		u4->reserved=bat->bat_data.u4.reserved;
	} else {
		FASTBAT_ERR_MSG(" Err! Wrong packet type!\n");
		retval=-EINVAL;
	}
	return	retval;
}

/* Ex. 
 * if(bat->soft_iface == NULL)
 *    //error code
 * fastbat_grab_priv_ttvn(bat->soft_iface);
 *
 * return priv->tt.vn
 *
 * grab current ttvn value from batman private area
 */
static u8	fastbat_grab_priv_ttvn
(struct net_device *netdev)
{
	//struct batadv_priv	*bat_priv;
	u8	vn=0;

	//bat_priv=netdev_priv(netdev);
	if(bat_priv == NULL)
		FASTBAT_ERR_MSG(" Err! Null bat_priv!\n");
	vn=(u8)atomic_read(&bat_priv->tt.vn);
	return	vn;
}

/* TODO: double vlan
 * return 0 if successful
 *    A success does not guarantee the frame will be transmitted as it may be dropped due
 *    to congestion or traffic shaping
 * return: A negative errno code is returned on a failure
 *    it drops and returns NET_XMIT_DROP(which is > 0). This will not be treated as an error
 */
int		fastbat_xmit
(struct sk_buff *skb, 
 FAST_BAT *bat)
{
	u16				prototype;
	int				retval=0;
	struct ethhdr	*ethhdr;
	struct vlan_ethhdr					*vhdr;
	struct batadv_unicast_packet		*u;
	struct batadv_unicast_4addr_packet	*u4;

	retval=fastbat_xmit_check(bat);
	if(retval < 0)
		goto	ERR;
	if(bat->vlan/* == 1*/) {
		vhdr=vlan_eth_hdr(skb);
		vhdr->h_vlan_TCI=bat->vlan_data.h_vlan_TCI;
		vhdr->h_vlan_encapsulated_proto=bat->vlan_data.h_vlan_encapsulated_proto;
		if(bat->batman)
			fastbat_clone_header((void *)bat, (void *)((struct vlan_ethhdr *)vhdr+1));
		else
			FASTBAT_DBG_MSG(" Wrong header!\n");
	} else if(bat->batman) {
		ethhdr=eth_hdr(skb);
		fastbat_clone_header((void *)bat, (void *)((struct ethhdr *)ethhdr+1));
	} else {
		//don't need to alter header
	}

	//skb_reset_mac_header(skb);
	//ethhdr=eth_hdr(skb);
	//ether_addr_copy(ethhdr->h_source, bat->out->net_dev->dev_addr);
	//ether_addr_copy(ethhdr->h_dest, bat->dst);
	
	skb_set_network_header(skb, ETH_HLEN);
	skb->dev=bat->net_dev;
	skb->protocol=htons(ETH_P_BATMAN);

	retval=dev_queue_xmit(skb);
	xxxFASTBAT_DBG_RAW_MSG("[F@STBAT] xmit(%d)!\n", retval);
	return	net_xmit_eval(retval);

ERR:
	FASTBAT_DBG_MSG(" Xmit failed! Back to batman!\n");
	return	retval;
}

/* @orig_node: an originator serving this client
 * @addr: the mac address of the client
 */
void	fastbat_check_client_and_setup_dirty
(struct batadv_orig_node *orig, 
 unsigned char *addr)
{
	int			idx=0;
	FAST_BAT	*bat;

	FASTBAT_DBG_RAW_MSG("[F@STBAT] setup dirty (o)%pM (c)%pM\n", orig, addr);

	/* global bats */
	for(; idx<FAST_BAT_LIST_LEN; idx++) {
		//if(!hlist_empty(&bats[idx])) {
			hlist_for_each_entry(bat, &bats[idx], list) {
				if(bat->dirty == 1)
					continue;
				if(!memcmp(bat->orig, orig->orig, ETH_ALEN)) {
					if(!memcmp(bat->src , addr, ETH_ALEN) || !memcmp(bat->dst, addr, ETH_ALEN)) {
						bat->dirty=1;
						FASTBAT_DBG_RAW_MSG("[F@STBAT] find out::(o)%pM\n (s)%pM\n (d)%pM\n (rs)%pM\n (rd)%pM\n", bat->orig, bat->src1, bat->dst1, bat->src, bat->dst);
						//fastbat_del_list_entry(bat);
					}
				}
			}
		//}
	}
}


/* @addr: the mac address of the client
 */
void	fastbat_check_local_client_and_setup_dirty
(u8 *addr)
{
	FAST_LOCAL  *local;
	FASTBAT_DBG_RAW_MSG("[F@STBAT] setup dirty (c)%pM\n", addr);
	/* local bats */
	list_for_each_entry(local, &fastbat_local_head, list) {
		if(!memcmp(local->dst, addr, ETH_ALEN)) {
			FASTBAT_DBG_RAW_MSG("[F@STBAT] find out local::(addr)%pM (dev)%s\n", 
				local->dst, local->output_dev->name);
			local->dirty=1;
		}
	}
}

void	fastbat_check_orig_and_setup_dirty
(struct batadv_orig_node *orig_node, 
 struct batadv_neigh_node *neigh_node)
{
	int			idx=0;
	FAST_BAT	*bat;

	FASTBAT_DBG_RAW_MSG("[F@STBAT] setup dirty (o)%pM (n)%pM\n", 
		orig_node->orig, neigh_node->addr);
	for(; idx<FAST_BAT_LIST_LEN; idx++) {
		//if(!hlist_empty(&bats[idx])) {
			hlist_for_each_entry(bat, &bats[idx], list) {
				if(bat->dirty == 1)
					continue;
				if(!memcmp(bat->orig, orig_node->orig, ETH_ALEN)) {
					bat->dirty=1;   /*dirty*/
					FASTBAT_DBG_RAW_MSG("[F@STBAT] find out:(o)%pM\n (s)%pM\n (d)%pM\n (rs)%pM\n (rd)%pM\n", bat->orig, bat->src1, bat->dst1, bat->src, bat->dst);
					//fastbat_del_list_entry(bat);
				}
			}
		//}
	}

#if	0//debug
	int	oidx=0;
	int	nidx=0;
	struct batadv_neigh_node	*tmp;
	struct batadv_orig_ifinfo	*tmp_orig_ifinfo;
	struct batadv_neigh_ifinfo	*tmp_neigh_ifinfo;
	FASTBAT_DBG_RAW_MSG("[F@STBAT] (o)%x:%x:%x:%x:%x:%x/%x  (n)%x:%x:%x:%x:%x:%x/%x\n~~~~~~ ~~~~~~\n", 
		orig[0], orig[1], orig[2], orig[3], orig[4], orig[5], orig_node,  
		neigh[0], neigh[1], neigh[2], neigh[3], neigh[4], neigh[5], neigh_node);
	if(!hlist_empty(&orig_node->neigh_list)) {

		hlist_for_each_entry(tmp, &orig_node->neigh_list, list) {
			FASTBAT_DBG_RAW_MSG("[F@STBAT] (to)%x:%x:%x:%x:%x:%x  (tn)%x:%x:%x:%x:%x:%x\n", 
				tmp->orig_node->orig[0], tmp->orig_node->orig[1], tmp->orig_node->orig[2], 
				tmp->orig_node->orig[3], tmp->orig_node->orig[4], tmp->orig_node->orig[5], 
				tmp->addr[0], tmp->addr[1], tmp->addr[2], 
				tmp->addr[3], tmp->addr[4], tmp->addr[5]);
		}

		hlist_for_each_entry(tmp_orig_ifinfo, &orig_node->ifinfo_list, list) {
			/*oidx++;
			if(tmp_orig_ifinfo->if_outgoing)
				FASTBAT_DBG_RAW_MSG("[F@STBAT] orig_ifinfo(%d) %s\n", 
					oidx, tmp_orig_ifinfo->if_outgoing->net_dev->name);
			else
				FASTBAT_DBG_RAW_MSG("[F@STBAT] orig_ifinfo(%d) iface is null \n", oidx);*/
			if(tmp_orig_ifinfo->router)
				FASTBAT_DBG_RAW_MSG("[F@STBAT] orig_ifinf (r)%x %x:%x:%x:%x:%x:%x\n", 
					tmp_orig_ifinfo->router, 
					tmp_orig_ifinfo->router->addr[0], tmp_orig_ifinfo->router->addr[1], 
					tmp_orig_ifinfo->router->addr[2], tmp_orig_ifinfo->router->addr[3], 
					tmp_orig_ifinfo->router->addr[4], tmp_orig_ifinfo->router->addr[5]);
			else {
				return;
				FASTBAT_DBG_RAW_MSG("[F@STBAT] orig_ifinfo router is null \n");
			}
			hlist_for_each_entry(tmp_neigh_ifinfo, &tmp_orig_ifinfo->router->ifinfo_list, list) {
				nidx++;
				if(tmp_neigh_ifinfo->if_outgoing)
					FASTBAT_DBG_RAW_MSG("[F@STBAT] neigh_ifinfo: %s(%d)\n", 
						tmp_neigh_ifinfo->if_outgoing->net_dev->name, nidx);
				else
					FASTBAT_DBG_RAW_MSG("[F@STBAT] neigh_ifinfo(%d) iface is null\n", nidx);
				FASTBAT_DBG_RAW_MSG("[F@STBAT] neigh_ifinfo throuput:%d\n", 
					tmp_neigh_ifinfo->bat_v.throughput);
			}
		}
	}
#endif
	
}

int		fastbat_show_all_bats
(struct seq_file *seq, 
 void *offset)
{
	FAST_BAT	*bat;
	FAST_LOCAL	*local;
	int			idx=0, 
				counts=0;

	if(!fastmesh_enable) {
		seq_puts(seq, "[F@STBAT] fast bat is off!\n");
		return	0;
	}

	/* bats */
	seq_puts(seq, "(No.)   [idx] <src>             <dst>             <realsrc>         <realdst>         <orig>            <dirty> <batman> <vlan> <batman-type> <dev>\n");
	for(; idx<FAST_BAT_LIST_LEN; idx++) {
		if(!hlist_empty(&bats[idx])) {
			hlist_for_each_entry(bat, &bats[idx], list) {
				++counts;
				seq_printf(seq, "(NO-%02d) [%02d]  %pM %pM %pM %pM %pM %d       %d        %d      0x%x          %s\n", 
					counts, idx, bat->src1, bat->dst1, bat->src, bat->dst, bat->orig, 
					bat->dirty, bat->batman, bat->vlan, bat->batman_type, bat->net_dev->name);
			}
		}
	}

	/* local bats */
	counts=0;
	seq_puts(seq, "(NO.)   <dst>             <dirty> <dev>\n");
	if(!list_empty(&fastbat_local_head)) {
		list_for_each_entry(local, &fastbat_local_head, list) {
			++counts;
			seq_printf(seq, "(NO-%02d) %pM %d       %s\n", 
				counts, local->dst, local->dirty, local->output_dev->name);
		}
	}
	return	0;
}

void	fastbat_update_batman_counters
()
{

}

