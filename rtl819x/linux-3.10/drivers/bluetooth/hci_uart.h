/*
 *
 *  Bluetooth HCI UART driver
 *
 *  Copyright (C) 2000-2001  Qualcomm Incorporated
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2004-2005  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <linux/version.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

/* #define HCI_VERSION_CODE KERNEL_VERSION(3, 14, 41) */
/* #define HCI_VERSION_CODE LINUX_VERSION_CODE */
#define HCI_VERSION_CODE KERNEL_VERSION(4, 4, 0)

#ifndef N_HCI
#define N_HCI	15
#endif

//#define BTCOEX

/* Ioctls */
#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTGETPROTO		_IOR('U', 201, int)
#define HCIUARTGETDEVICE	_IOR('U', 202, int)
#define HCIUARTSETFLAGS		_IOW('U', 203, int)
#define HCIUARTGETFLAGS		_IOR('U', 204, int)

/* UART protocols */
#define HCI_UART_MAX_PROTO	6

#define HCI_UART_H4	0
#define HCI_UART_BCSP	1
#define HCI_UART_3WIRE	2
#define HCI_UART_H4DS	3
#define HCI_UART_LL	4
#define HCI_UART_ATH3K	5

#define HCI_UART_RAW_DEVICE	0
#define HCI_UART_RESET_ON_INIT	1
#define HCI_UART_CREATE_AMP	2
#define HCI_UART_INIT_PENDING	3
#define HCI_UART_EXT_CONFIG	4
#define HCI_UART_VND_DETECT	5

struct hci_uart;

struct hci_uart_proto {
	unsigned int id;
	int (*open)(struct hci_uart *hu);
	int (*close)(struct hci_uart *hu);
	int (*flush)(struct hci_uart *hu);
	int (*recv)(struct hci_uart *hu, void *data, int len);
	int (*enqueue)(struct hci_uart *hu, struct sk_buff *skb);
	struct sk_buff *(*dequeue)(struct hci_uart *hu);
};

struct hci_uart {
	struct tty_struct	*tty;
	struct hci_dev		*hdev;
	unsigned long		flags;
	unsigned long		hdev_flags;
#ifdef CONFIG_ZYXEL_BLE_PATCH
    struct work_struct	write_work;
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
	struct work_struct	write_work;
#endif
#endif /* CONFIG_ZYXEL_BLE_PATCH */

	struct hci_uart_proto	*proto;
	void			*priv;

	struct sk_buff		*tx_skb;
	unsigned long		tx_state;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)
	spinlock_t		rx_lock;
#endif
};

/* HCI_UART proto flag bits */
#define HCI_UART_PROTO_SET	0
#define HCI_UART_REGISTERED	1
#define HCI_UART_PROTO_READY	2

/* TX states  */
#define HCI_UART_SENDING	1
#define HCI_UART_TX_WAKEUP	2

extern int hci_uart_register_proto(struct hci_uart_proto *p);
extern int hci_uart_unregister_proto(struct hci_uart_proto *p);
extern int hci_uart_tx_wakeup(struct hci_uart *hu);

#ifdef CONFIG_BT_HCIUART_H4
extern int h4_init(void);
extern int h4_deinit(void);
#endif

extern int h5_init(void);
extern int h5_deinit(void);

#if HCI_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
extern int hci_uart_send_frame(struct sk_buff *skb);
#else
extern int hci_uart_send_frame(struct hci_dev *hdev, struct sk_buff *skb);
#endif
