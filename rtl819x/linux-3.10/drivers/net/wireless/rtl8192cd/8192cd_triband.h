#ifndef _8192CD_TRIBNAD_H_
#define _8192CD_TRIBNAD_H_


#if defined(_8192CD_TRIBNAD_UTIL_LOCK_H_)

//#define TRIBAND_LOCK_DEBUG

#define SAVE_INT_AND_CLI(__x__)		do { } while (0)
#define RESTORE_INT(__x__)			do { } while (0)

#define SMP_LOCK(__x__)	\
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			if(priv->pshare->lock_owner!=smp_processor_id()) \
				spin_lock_irqsave(&priv->pshare->lock, priv->pshare->lock_flags); \
			else {\
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->pshare->lock_func); \
				dump_stack(); panic_printk("<<<<<===\n\n"); \
			} \
			strcpy(priv->pshare->lock_func, __FUNCTION__);\
			priv->pshare->lock_owner=smp_processor_id();\
		} \
	}while(0)

#define SMP_UNLOCK(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->pshare->lock_owner=-1; \
			spin_unlock_irqrestore(&priv->pshare->lock, priv->pshare->lock_flags); \
		} \
	}while(0)

#define SMP_TRY_LOCK(__x__,__y__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			if(priv->pshare->lock_owner != smp_processor_id()) { \
				SMP_LOCK(__x__); \
				__y__ = 1; \
			} else \
				__y__ = 0; \
		} \
	} while(0)

#define SMP_LOCK_ASSERT() \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			if(priv->pshare->lock_owner!=smp_processor_id()) { \
					panic_printk("ERROR: Without obtaining SMP_LOCK(). Please calling SMP_LOCK() before entering into %s()\n\n\n",__FUNCTION__); \
					return; \
			} \
		} \
	}while(0)

#define SMP_LOCK_XMIT(__x__)	\
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			if(priv->pshare->lock_xmit_owner!=smp_processor_id()) \
				spin_lock_irqsave(&priv->pshare->lock_xmit, priv->pshare->lock_xmit_flags); \
			else {\
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->pshare->lock_xmit_func); \
				dump_stack(); panic_printk("<<<<<===\n\n"); \
			}\
			strcpy(priv->pshare->lock_xmit_func, __FUNCTION__);\
			priv->pshare->lock_xmit_owner=smp_processor_id();\
		} \
	}while(0)

#define SMP_UNLOCK_XMIT(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->pshare->lock_xmit_owner=-1; \
			spin_unlock_irqrestore(&priv->pshare->lock_xmit, priv->pshare->lock_xmit_flags); \
		} \
	}while(0)

#define SMP_TRY_LOCK_XMIT(__x__,__y__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			if(priv->pshare->lock_xmit_owner != smp_processor_id()) { \
				SMP_LOCK_XMIT(__x__); \
				__y__ = 1; \
			} else \
				__y__ = 0; \
			} \
	} while(0)

#ifdef TRIBAND_LOCK_DEBUG
#define SMP_LOCK_HASH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			__u32 _cpu_id = get_cpu(); \
			if (priv->hash_list_lock_owner != _cpu_id) { \
				spin_lock_irqsave(&priv->hash_list_lock, (__x__)); \
			} \
			else { \
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->hash_list_lock_func); \
				dump_stack(); \
			} \
			strcpy(priv->hash_list_lock_func, __FUNCTION__); \
			priv->hash_list_lock_owner = _cpu_id; \
			put_cpu(); \
		} \
		else { \
			spin_lock_bh(&priv->hash_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_HASH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->hash_list_lock_owner = -1; \
			spin_unlock_irqrestore(&priv->hash_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->hash_list_lock); (void)(__x__); \
		} \
	} while(0)

#else //ori

#define SMP_LOCK_HASH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->hash_list_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->hash_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_HASH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->hash_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->hash_list_lock); (void)(__x__); \
		} \
	} while(0)

#endif //TRIBAND_LOCK_DEBUG

#define SMP_LOCK_STACONTROL_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->stactrl.stactrl_lock, (__x__)); \
		} \
	} while(0)

#define SMP_UNLOCK_STACONTROL_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->stactrl.stactrl_lock, (__x__)); \
		} \
	} while(0)

#define SMP_LOCK_SR_BLOCK_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->sr_block.sr_block_lock, (__x__)); \
		} \
	} while(0)

#define SMP_UNLOCK_SR_BLOCK_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->sr_block.sr_block_lock, (__x__)); \
		} \
	} while(0)

#define SMP_LOCK_ACL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_lock(&priv->wlan_acl_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_ACL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_unlock(&priv->wlan_acl_list_lock); (void)(__x__); \
		} \
	} while(0)

#ifdef TRIBAND_LOCK_DEBUG
#define SMP_LOCK_ASOC_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			__u32 _cpu_id = get_cpu(); \
			if (priv->asoc_list_lock_owner != _cpu_id) { \
				spin_lock_irqsave(&priv->asoc_list_lock, (__x__)); \
			} \
			else { \
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->asoc_list_lock_func); \
				dump_stack(); \
			} \
			strcpy(priv->asoc_list_lock_func, __FUNCTION__); \
			priv->asoc_list_lock_owner = _cpu_id; \
			put_cpu(); \
		} \
		else { \
			spin_lock_bh(&priv->asoc_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_ASOC_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->asoc_list_lock_owner = -1; \
			spin_unlock_irqrestore(&priv->asoc_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->asoc_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_SLEEP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			__u32 _cpu_id = get_cpu(); \
			if (priv->sleep_list_lock_owner != _cpu_id) { \
				spin_lock_irqsave(&priv->sleep_list_lock, (__x__)); \
			} \
			else { \
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->sleep_list_lock_func); \
				dump_stack(); \
			} \
			strcpy(priv->sleep_list_lock_func, __FUNCTION__); \
			priv->sleep_list_lock_owner = _cpu_id; \
			put_cpu(); \
		} \
		else { \
			spin_lock_bh(&priv->sleep_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_SLEEP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->sleep_list_lock_owner = -1; \
			spin_unlock_irqrestore(&priv->sleep_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->sleep_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_AUTH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			__u32 _cpu_id = get_cpu(); \
			if (priv->auth_list_lock_owner != _cpu_id) { \
				spin_lock_irqsave(&priv->auth_list_lock, (__x__)); \
			} \
			else { \
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->auth_list_lock_func); \
				dump_stack(); \
			} \
			strcpy(priv->auth_list_lock_func, __FUNCTION__); \
			priv->auth_list_lock_owner = _cpu_id; \
			put_cpu(); \
		} \
		else { \
			spin_lock_bh(&priv->auth_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_AUTH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->auth_list_lock_owner = -1; \
			spin_unlock_irqrestore(&priv->auth_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->auth_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_WAKEUP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			__u32 _cpu_id = get_cpu(); \
			if (priv->wakeup_list_lock_owner != _cpu_id) { \
				spin_lock_irqsave(&priv->wakeup_list_lock, (__x__)); \
			} \
			else { \
				panic_printk("[%s %d] recursion detection, dev=%s, caller=%p\n",__FUNCTION__,__LINE__,priv->dev->name,__builtin_return_address(0)); \
				panic_printk("Previous Lock Function is %s\n",priv->wakeup_list_lock_func); \
				dump_stack(); \
			} \
			strcpy(priv->wakeup_list_lock_func, __FUNCTION__); \
			priv->wakeup_list_lock_owner = _cpu_id; \
			put_cpu(); \
		} \
		else { \
			spin_lock_bh(&priv->wakeup_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_WAKEUP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			priv->wakeup_list_lock_owner = -1; \
			spin_unlock_irqrestore(&priv->wakeup_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->wakeup_list_lock); (void)(__x__); \
		} \
	} while(0)

#else //ori

#define SMP_LOCK_ASOC_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->asoc_list_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->asoc_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_ASOC_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->asoc_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->asoc_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_SLEEP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->sleep_list_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->sleep_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_SLEEP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->sleep_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->sleep_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_AUTH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->auth_list_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->auth_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_AUTH_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->auth_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->auth_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_WAKEUP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->wakeup_list_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->wakeup_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_WAKEUP_LIST(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->wakeup_list_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->wakeup_list_lock); (void)(__x__); \
		} \
	} while(0)

#endif //TRIBAND_LOCK_DEBUG

#define SMP_LOCK_MESH_MP_HDR(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_lock_bh(&priv->mesh_mp_hdr_lock); \
		} \
	} while(0)

#define SMP_UNLOCK_MESH_MP_HDR(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_unlock_bh(&priv->mesh_mp_hdr_lock); \
		} \
	} while(0)

#define SMP_LOCK_MESH_ACL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_lock(&priv->mesh_acl_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_UNLOCK_MESH_ACL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			spin_unlock(&priv->mesh_acl_list_lock); (void)(__x__); \
		} \
	} while(0)

#define SMP_LOCK_SKB(__x__) \
	do { \
		spin_lock_irqsave(&priv->pshare->lock_skb, (__x__)); \
	} while(0)

#define SMP_UNLOCK_SKB(__x__) \
	do { \
		spin_unlock_irqrestore(&priv->pshare->lock_skb, (__x__)); \
	} while(0)

#define SMP_LOCK_BUF(__x__) \
	do { \
		spin_lock_irqsave(&priv->pshare->lock_buf, (__x__)); \
	} while(0)

#define SMP_UNLOCK_BUF(__x__) \
	do { \
		spin_unlock_irqrestore(&priv->pshare->lock_buf, (__x__)); \
	} while(0)

#define SMP_LOCK_RECV(__x__)	\
	do { \
		if(priv->pshare->lock_recv_owner!=smp_processor_id()) \
			spin_lock_irqsave(&priv->pshare->lock_recv, (__x__)); \
		else { \
			panic_printk("[%s %d] recursion detection\n",__FUNCTION__,__LINE__); \
			dump_stack(); panic_printk("<<<<<===\n\n"); \
		} \
		priv->pshare->lock_recv_owner=smp_processor_id();\
	}while(0)

#define SMP_UNLOCK_RECV(__x__) \
	do { \
		priv->pshare->lock_recv_owner=-1; \
		spin_unlock_irqrestore(&priv->pshare->lock_recv, (__x__)); \
	}while(0)

#define SMP_TRY_LOCK_RECV(__x__,__y__) \
	do { \
		if(priv->pshare->lock_recv_owner != smp_processor_id()) { \
			SMP_LOCK_RECV(__x__); \
			__y__ = 1; \
		} else \
			__y__ = 0; \
	} while(0)

#define SMP_LOCK_RX_DATA(__x__) \
	do { \
		spin_lock_irqsave(&priv->rx_datalist_lock, (__x__)); \
	} while(0)

#define SMP_UNLOCK_RX_DATA(__x__) \
	do { \
		spin_unlock_irqrestore(&priv->rx_datalist_lock, (__x__)); \
	} while(0)

#define SMP_LOCK_RX_MGT(__x__) \
	do { \
		spin_lock_irqsave(&priv->rx_mgtlist_lock, (__x__)); \
	} while(0)

#define SMP_UNLOCK_RX_MGT(__x__) \
	do { \
		spin_unlock_irqrestore(&priv->rx_mgtlist_lock, (__x__)); \
	} while(0)

#define SMP_LOCK_RX_CTRL(__x__) \
	do { \
		spin_lock_irqsave(&priv->rx_ctrllist_lock, (__x__)); \
	} while(0)

#define SMP_UNLOCK_RX_CTRL(__x__) \
	do { \
		spin_unlock_irqrestore(&priv->rx_ctrllist_lock, (__x__)); \
	} while(0)

#define SMP_LOCK_REORDER_CTRL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->rc_packet_q_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->rc_packet_q_lock); (void)(__x__); \
		} \
	} while (0)

#define SMP_UNLOCK_REORDER_CTRL(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->rc_packet_q_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->rc_packet_q_lock); (void)(__x__); \
		} \
	} while (0)

#define DEFRAG_LOCK(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->defrag_lock, (__x__)); \
		} \
		else { \
			spin_lock_bh(&priv->defrag_lock); (void)(__x__); \
		} \
	} while (0)

#define DEFRAG_UNLOCK(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->defrag_lock, (__x__)); \
		} \
		else { \
			spin_unlock_bh(&priv->defrag_lock); (void)(__x__); \
		} \
	} while (0)

#define SMP_LOCK_PSK_RESEND(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->psk_resend_lock, (__x__)); \
		} \
	} while (0)

#define SMP_UNLOCK_PSK_RESEND(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->psk_resend_lock, (__x__)); \
		} \
	} while (0)

#define SMP_LOCK_PSK_GKREKEY(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_lock_irqsave(&priv->psk_gkrekey_lock, (__x__)); \
		} \
	} while (0)

#define SMP_UNLOCK_PSK_GKREKEY(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_PCIE) { \
			spin_unlock_irqrestore(&priv->psk_gkrekey_lock, (__x__)); \
		} \
	} while (0)

#define SMP_LOCK_MBSSID(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			_enter_critical_mutex(&priv->pshare->mbssid_lock, (__x__)); \
		} \
	} while (0)

#define SMP_UNLOCK_MBSSID(__x__) \
	do { \
		if (priv->hci_type == RTL_HCI_USB) { \
			_exit_critical_mutex(&priv->pshare->mbssid_lock, (__x__)); \
		} \
	} while (0)

#endif //_8192CD_TRIBNAD_UTIL_LOCK_H_


//////////////////////////////////////////////////////////////////////////////////
#if 0//defined(_8192CD_TRIBNAD_UTIL_RW_H_)
static inline unsigned char __HAL_RTL_R8(struct rtl8192cd_priv *priv, u32 addr)
{
	unsigned char ret = 0;
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		ret = (( priv->pwr_state==L2  || priv->pwr_state==L1) ? 0 :(RTL_R8_F(priv, reg)) );
		#else
		ret = RTL_R8_F(priv, addr);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		ret = usb_read8(priv, addr);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		ret = sdio_read8(priv, addr);
		break;
#endif
	}
	return ret;
}

static inline unsigned short __HAL_RTL_R16(struct rtl8192cd_priv *priv, u32 addr)
{
	unsigned short ret = 0;
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		ret = (( priv->pwr_state==L2  || priv->pwr_state==L1) ? 0 : (RTL_R16_F(priv, reg)));
		#else
		ret = RTL_R16_F(priv, addr);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		ret = usb_read16(priv, addr);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		ret = sdio_read16(priv, addr);
		break;
#endif
	}
	return ret;
}

static inline unsigned int __HAL_RTL_R32(struct rtl8192cd_priv *priv, u32 addr)
{
	unsigned int ret = 0;
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		ret = (( priv->pwr_state==L2  || priv->pwr_state==L1) ? 0 : (RTL_R32_F(priv, reg)));
		#else
		ret = RTL_R32_F(priv, addr);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		ret = usb_read32(priv, addr);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		ret = sdio_read32(priv, addr);
		break;
#endif
	}
	return ret;
}

static inline void __HAL_RTL_W8(struct rtl8192cd_priv *priv, u32 addr, u32 val)
{	
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		if( priv->pwr_state==L2  || priv->pwr_state==L1)
		{  	printk("Error!!! w8:%x,%x in L%d\n", addr, val, priv->pwr_state);}
		else
			RTL_W8_F(priv, addr, val);
		#else
		RTL_W8_F(priv, addr, val);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		usb_write8(priv, addr, val);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		sdio_read8(priv, addr, val);
		break;
#endif
	}
}

static inline void __HAL_RTL_W16(struct rtl8192cd_priv *priv, u32 addr, u32 val)
{
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		if( priv->pwr_state==L2  || priv->pwr_state==L1)
		{  	printk("Error!!! w16:%x,%x in L%d\n", addr, val, priv->pwr_state);}
		else
			RTL_W16_F(priv, addr, val);
		#else
		RTL_W16_F(priv, addr, val);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		usb_write16(priv, addr, val);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		sdio_write16(priv, addr, val);
		break;
#endif
	}
}

static inline void __HAL_RTL_W32(struct rtl8192cd_priv *priv, u32 addr, u32 val)
{
	switch (priv->hci_type) {
#if defined(CONFIG_PCI_HCI)
	case RTL_HCI_PCIE:
		#ifdef PCIE_POWER_SAVING_TEST //yllin
		if( priv->pwr_state==L2  || priv->pwr_state==L1)
		{  	printk("Error!!! w32:%x,%x in L%d\n", addr, val, priv->pwr_state);}
		else
			RTL_W32_F(priv, addr, val);
		#else
		RTL_W32_F(priv, addr, val);
		#endif
		break;
#endif
#if defined(CONFIG_USB_HCI)
	case RTL_HCI_USB:
		usb_write32(priv, addr, val);
		break;
#endif
#if defined(CONFIG_SDIO_HCI)
	case RTL_HCI_SDIO:
		sdio_write32(priv, addr, val);
		break;
#endif
	}
}

#define RTL_R8(reg)		\
    (__HAL_RTL_R8(priv, reg))

#define RTL_R16(reg)	\
    (__HAL_RTL_R16(priv, reg))

#define RTL_R32(reg)	\
    (__HAL_RTL_R32(priv, reg))

#define RTL_W8(reg, val8)	\
    do { \
        __HAL_RTL_W8(priv, reg, val8); \
    } while (0)

#define RTL_W16(reg, val16)	\
    do { \
        __HAL_RTL_W16(priv, reg, val16); \
    } while (0)

#define RTL_W32(reg, val32)	\
    do { \
        __HAL_RTL_W32(priv, reg, val32) ; \
    } while (0)

#define get_desc(val)	le32_to_cpu(val)
#define set_desc(val)	cpu_to_le32(val)
#endif //_8192CD_TRIBNAD_UTIL_RW_H_

//////////////////////////////////////////////////////////////////////////////////

#endif //_8192CD_TRIBNAD_H_