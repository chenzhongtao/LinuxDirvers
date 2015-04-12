/* �ο� 
 * drivers\mtd\nand\s3c2410.c
 * drivers\mtd\nand\at91_nand.c
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/sched.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/mach-types.h>

#include <asm/arch/nand.h>
#include <asm/arch/regs-nand.h>

/* S3C2416��nand�Ĵ������� */
struct s3c_nand_regs {
	unsigned long nfconf  ;
	unsigned long nfcont  ;
	unsigned long nfcmmd   ;
	unsigned long nfaddr  ;
	unsigned long nfdata  ;
	unsigned long nfeccd0 ;
	unsigned long nfeccd1 ;
	unsigned long nfeccd  ;
	unsigned long nfsblk;
	unsigned long nfeblk;
	unsigned long nfstat  ;
	unsigned long nfeccerr0;
	unsigned long nfeccerr1;
	unsigned long nfmecc0 ;
	unsigned long nfmecc1 ;
	unsigned long nfsecc  ;
};
	
static struct nand_chip *s3c_nand;
static struct mtd_info *s3c_mtd;
static struct s3c_nand_regs *s3c_nand_regs;

static void s3c2416_select_chip(struct mtd_info *mtd, int chipnr)
{
	if (chipnr == -1)
	{
		/* ȡ��ѡ��: NFCONT[1]��Ϊ1 */
		s3c_nand_regs->nfcont |= (1 << 1);
	}
	else
	{
		/* ѡ��: NFCONT[1]��Ϊ0 */
		s3c_nand_regs->nfcont &= ~(1 << 1);
	}
}

static void s3c2416_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (ctrl & NAND_CLE)
	{
		/* ������: NFCMMD=dat */
		s3c_nand_regs->nfcmmd = cmd;
	}
	else
	{
		/* ����ַ: NFADDR=dat */
		s3c_nand_regs->nfaddr = cmd;
	}
}

/*
 * ����1��ʾready,0��ʾbusy
 */
int s3c2416_dev_ready(struct mtd_info *mtd)
{
	return (s3c_nand_regs->nfstat & (1<<0));
}

static int s3c_nand_init(void)
{
	struct clk *clk;
	
	/* 1. ����һ��nand_chip�ṹ�� */
	s3c_nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	s3c_nand_regs = ioremap(0x4E000000, sizeof(struct s3c_nand_regs));
	
	/* 2. ����nand_chip */
	/* ����nand_chip�Ǹ�nand_scan����ʹ�õ�, �����֪����ô����, �ȿ�nand_scan��ôʹ�� 
	 * ��Ӧ���ṩ:ѡ��,������,����ַ,������,������,�ж�״̬�Ĺ���
	 */
	s3c_nand->select_chip = s3c2416_select_chip;
	s3c_nand->cmd_ctrl = s3c2416_cmd_ctrl;
	s3c_nand->IO_ADDR_R = &s3c_nand_regs->nfdata;
	s3c_nand->IO_ADDR_W = &s3c_nand_regs->nfdata;
	s3c_nand->dev_ready = s3c2416_dev_ready;
	s3c_nand->ecc.mode = NAND_ECC_SOFT;	/* enable ECC��ʹ��ECC������insmodʱ�о��� */
										/* NAND_ECC_NONE selected by board driver. This is not recommended !! */
	
	/* 3. Ӳ����ص�����: ����NAND FLASH���ֲ�����ʱ����� */
	/* ʹ��NAND FLASH��������ʱ�� */
	clk = clk_get(NULL, "nand");
	clk_enable(clk);              /* clock.c��"nand"��enable�����ǿյ�???? */
	
	/* HCLK=133MHz
	 * TACLS:  ����CLE/ALE֮��೤ʱ��ŷ���nWE�ź�, ��NAND�ֲ��֪CLE/ALE��nWE����ͬʱ����,����TACLS=0
	 * TWRPH0: nWE��������, HCLK x ( TWRPH0 + 1 ), ��NAND�ֲ�(Tcls)��֪��Ҫ>=15ns, ����TWRPH0>=1
	 * TWRPH1: nWE��Ϊ�ߵ�ƽ��೤ʱ��CLE/ALE���ܱ�Ϊ�͵�ƽ, ��NAND�ֲ�(Tclh)��֪��Ҫ>=5ns, ����TWRPH1>=0
	 */
#define TACLS   	0x02	//0x00
#define TWRPH0  0x0f	//0x01
#define TWRPH1  0x07	//0x00
	s3c_nand_regs->nfconf = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);

	/* NFCONT: 
	 * BIT1-��Ϊ1, ȡ��Ƭѡ 
	 * BIT0-��Ϊ1, ʹ��NAND FLASH������
	 */
	s3c_nand_regs->nfcont = (1<<1) | (1<<0);

	/* 4. ʹ��: nand_scan */
	s3c_mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	
	s3c_nand->priv = s3c_mtd;		/* link the private data structures */
	s3c_mtd->priv = s3c_nand;
	s3c_mtd->owner = THIS_MODULE;
	
	nand_scan(s3c_mtd, 1);		/* ʶ��NAND FLASH, ����mtd_info */
	
	/* 5. add_mtd_partitions */

	return 0;
}


static void s3c_nand_exit(void)
{
	kfree(s3c_mtd);
	iounmap(s3c_nand_regs);
	kfree(s3c_nand);
}

module_init(s3c_nand_init);
module_exit(s3c_nand_exit);

MODULE_LICENSE("GPL");