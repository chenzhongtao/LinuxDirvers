/*
 * 参考 drivers\net\cs89x0.c
 */

#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *vnet_dev;

static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
	/* 参考LDD3,snull */
	unsigned char *type;
	struct iphdr *ih;
	__be32 *saddr, *daddr, tmp;
	unsigned char	tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;
	
	struct sk_buff *rx_skb;
		
	// 从硬件读出/保存数据
	/* 对调"源/目的"的mac地址 */
	ethhdr = (struct ethhdr *)skb->data;
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

	/* 对调"源/目的"的ip地址 */    
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;
	
	//((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
	//((u8 *)daddr)[2] ^= 1;
	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
	//printk("tx package type = %02x\n", *type);
	// 修改类型, 原来0x8表示ping
	*type = 0; /* 0表示reply */
	
	ih->check = 0;		   /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
	
	// 构造一个sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);	/* 实际是调用alloc_skb(x+16)，分配实际缓冲区和sk_buff结构 */
	/* 由于以太网帧头是14字节，这里先保留2字节，这样就做到16字节对齐 */
	skb_reserve(rx_skb, 2); /* align IP on 16B boundary */	
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;	/* 接收封包时，skb->dev代表接收该封包的设备的指针
						 * 发送封包时，skb->dev代表发送该封包的设备的指针
						 */

	/* 在netif_rx之前必须确定skb->protocol的值，即网络层的协议代号 */
	rx_skb->protocol = eth_type_trans(rx_skb, dev);	/* determine the packet's protocol ID */
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	// 提交sk_buff
	netif_rx(rx_skb);

}
static int virt_net_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	static int cnt = 0;
	printk("virt_net_send_packet cnt = %d\n", ++cnt);
	
	/* 对于真实的网卡, 把skb里的数据通过网卡发送出去 */
	netif_stop_queue(dev);	 /* 停止该网卡的队列 */
	/* ...... */           /* 把skb的数据写入网卡 */

	/* 打印IP帧头的第一个字节，为0x45。低4位是首部长度，高4位是版本号 */
	printk("skb->iphdr[0] = 0x%02x\n", *(skb->data + sizeof(struct ethhdr)));
	
	/* 构造一个假的sk_buff,上报 */
	emulator_rx_packet(skb, dev);
	
	dev_kfree_skb (skb);		/* 释放skb，实际调用kfree_skb() */
	/* 数据全部发送出去后,唤醒网卡的队列 
	 * 应该是放在发送完成中断里，参考cs89x0.c
	 */
	netif_wake_queue(dev);	/* Inform upper layers. */

	/* 更新统计信息 */
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;	/* 这里是98字节，MAC:14,IP:20,ICMP:8+56。数据中不包含以太网帧4字节CRC */
	
	return 0;
}

static int virt_net_init(void)
{
	printk("virt_net_init\n");
	/* 1. 分配一个net_device结构体 */
	vnet_dev = alloc_netdev(0, "vnet%d", ether_setup);

	/* 2. 设置 */
	/* 设置发包函数 */
	vnet_dev->hard_start_xmit = virt_net_send_packet;

	/* 设置MAC地址 */
	vnet_dev->dev_addr[0] = 0x10;
	vnet_dev->dev_addr[1] = 0x11;
	vnet_dev->dev_addr[2] = 0x12;
	vnet_dev->dev_addr[3] = 0x13;
	vnet_dev->dev_addr[4] = 0x14;
	vnet_dev->dev_addr[5] = 0x15;

	/* 设置下面两项才能ping通 */
	vnet_dev->flags |= IFF_NOARP;
	vnet_dev->features |= NETIF_F_NO_CSUM;	
	
	/* 3. 注册 */
	/* register_netdev()里先rtnl_lock()，然后调用register_netdevice()。
	 * register_netdevice()先rtnl_trylock()，如果成功，说明未调用rtnl_lock()，
	 * 然后打印错误信息。
	 * 注册网络设备要调用register_netdev()，而不是register_netdevice()
	 */
	//register_netdevice(vnet_dev);
	register_netdev(vnet_dev);

	return 0;
}


static void virt_net_exit(void)
{
//	unregister_netdevice(vnet_dev);
	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}


module_init(virt_net_init);
module_exit(virt_net_exit);

MODULE_AUTHOR("dazuo.com");
MODULE_LICENSE("GPL");

