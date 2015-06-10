#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/spi/spi.h>

#include <linux/fs.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>

#include <linux/delay.h>

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/mtd/compatmac.h>
#include <linux/mtd/mtd.h>

#include <linux/sched.h>


#define CMD_AAI       		0xAD  		/* AAI �������ָ��(FOR SST25VF016B) */
#define CMD_DISWR	  	0x04		/* ��ֹд, �˳�AAI״̬ */
#define CMD_EWRSR	  	0x50		/* ����д״̬�Ĵ��������� */
#define CMD_WRSR      	0x01  		/* д״̬�Ĵ������� */
#define CMD_WRDISEN	0x04		/* ��ֹдʹ������ */
#define CMD_WREN      	0x06		/* дʹ������ */
#define CMD_READ      	0x03  		/* ������������ */
#define CMD_RDSR1      	0x05		/* ��״̬�Ĵ���1���� */
#define CMD_RDSR2      	0x35		/* ��״̬�Ĵ���2���� */
#define CMD_RDID      	0x9F		/* ������ID���� */
#define CMD_SE        		0x20		/* ������������ */
#define CMD_BE        		0xC7		/* ������������ */
#define CMD_PAGE_PROGRAM	0x02	/* ҳд�� */
#define DUMMY_BYTE    	0xA5		/* ���������Ϊ����ֵ�����ڶ����� */

#define WIP_FLAG      	0x01		/* ״̬�Ĵ����е����ڱ�̱�־��WIP) */


/* ����ע�� spi_driver */
static struct spi_device *spi_flash;

/* ����ע�� mtd_info */
static struct mtd_info spi_flash_dev;


/* �ο�:
 * drivers/mtd/devices/mtdram.c
 * drivers/mtd/devices/m25p80.c
 */

/*
*********************************************************************************************************
*	�� �� ��: spi_flash_write_enable
*	����˵��: ����������дʹ������
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void spi_flash_write_enable(void)
{
	unsigned char tx_buf[1] = {CMD_WREN};
	spi_write(spi_flash, tx_buf, 1);
}

/*
*********************************************************************************************************
*	�� �� ��: spi_flash_write_disable
*	����˵��: ���������ͽ�ֹдʹ������
*	��    ��:  ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
static void spi_flash_write_disable(void)
{
	unsigned char tx_buf[1] = {CMD_WRDISEN};
	spi_write(spi_flash, tx_buf, 1);
}

static unsigned char spi_flash_read_status_reg1(void)
{
	unsigned char tx_buf[1] = {CMD_RDSR1};
	unsigned char rx_buf[1] = {0};

	spi_write_then_read(spi_flash, tx_buf, 1, rx_buf, 1);

	return rx_buf[0];
}

static unsigned char spi_flash_read_status_reg2(void)
{
	unsigned char tx_buf[1] = {CMD_RDSR2};
	unsigned char rx_buf[1] = {0};

	spi_write_then_read(spi_flash, tx_buf, 1, rx_buf, 1);

	return rx_buf[0];
}

static void spi_flash_wait_for_busy(void)
{
	while((spi_flash_read_status_reg1() & 0x01) != 0)
	{
		/* ����һ��ʱ�� */
		/* Sector erase time : 60ms
		 * Page program time : 0.7ms
		 * Write status reg time : 10ms
		 */
		set_current_state(TASK_INTERRUPTIBLE);
        	schedule_timeout(HZ/100);  /* ����10MS���ٴ��ж� */
	}
}

static void spi_flash_write_status_reg(unsigned char reg1, unsigned char reg2)
{
	unsigned char tx_buf[3] = {CMD_WRSR, reg1, reg2};
	
	/* д��Write enableָ�����д��status reg */
	spi_flash_write_enable();

	spi_write(spi_flash, tx_buf, 3);
	spi_flash_wait_for_busy();
}

static void spi_flash_status_reg_protect_off(void)
{
	unsigned char reg1, reg2;

	reg1 = spi_flash_read_status_reg1();
	reg2 = spi_flash_read_status_reg2();

	reg1 &= ~(1 << 7);
	reg2 &= ~(1 << 0);

	spi_flash_write_status_reg(reg1, reg2);
}

static void spi_flash_chip_protect_off(void)
{
	/* cmp = 0, bp2=bp1=bp0=0 */
	
	unsigned char reg1, reg2;

	reg1 = spi_flash_read_status_reg1();
	reg2 = spi_flash_read_status_reg2();

	reg1 &= ~((1 << 2) | (1 << 3) | (1 << 4));
	reg2 &= ~(1 << 6);

	spi_flash_write_status_reg(reg1, reg2);
}

/* ����4kb */
void spi_flash_erase_sector(unsigned int addr)
{
	unsigned char tx_buf[4] = {0};

	tx_buf[0] = CMD_SE;
	tx_buf[1] = (addr >> 16) & 0xFF;
	tx_buf[2] = (addr >> 8) & 0xFF;
	tx_buf[3] = (addr >> 0) & 0xFF;
	
	spi_flash_write_enable();

	spi_write(spi_flash, tx_buf, 4);

	spi_flash_wait_for_busy();
}

void spi_flash_page_program(unsigned int addr, unsigned char *buf, unsigned int len)
{
	unsigned char tx_buf[4];   
	struct spi_transfer	t[] = {
		{
			.tx_buf	= tx_buf,
			.len		= 4,
		},
		{
			.tx_buf	= buf,
			.len		= len,
		},
	};
	struct spi_message m;

	tx_buf[0] = CMD_PAGE_PROGRAM;
	tx_buf[1] = addr >> 16;
	tx_buf[2] = addr >> 8;
	tx_buf[3] = addr & 0xff;
	
	spi_flash_write_enable();

	spi_message_init(&m);
	spi_message_add_tail(&t[0], &m);
	spi_message_add_tail(&t[1], &m);
	spi_sync(spi_flash, &m);

	spi_flash_wait_for_busy();
}


void spi_flash_read(unsigned int addr, unsigned char *buf, unsigned int len)
{
    /* spi_write_then_read�涨��tx_cnt+rx_cnt < 32
     * ���Զ��ڴ������ݵĶ�ȡ������ʹ�øú���
     */
     
	unsigned char tx_buf[4];   
	struct spi_transfer	t[] = 
	{
		{
    			.tx_buf	= tx_buf,
    			.len		= 4,
		},
		{
    			.rx_buf	= buf,
    			.len		= len,
		},
	};
	struct spi_message m;

	tx_buf[0] = CMD_READ;
	tx_buf[1] = addr >> 16;
	tx_buf[2] = addr >> 8;
	tx_buf[3] = addr & 0xff;

	spi_message_init(&m);
	spi_message_add_tail(&t[0], &m);
	spi_message_add_tail(&t[1], &m);
	spi_sync(spi_flash, &m); 
	
}

static void spi_flash_read_id(unsigned int *pMID, unsigned int *pDID)
{
	unsigned char tx_buf[1] = {0};
	unsigned char rx_buf[3] = {0};

	tx_buf[0] = CMD_RDID;

	spi_write_then_read(spi_flash, tx_buf, 1, rx_buf, 3);

	*pMID = rx_buf[0];
	*pDID = rx_buf[1];
	*pDID = (*pDID << 8) | rx_buf[2];
}

static int spi_flash_erase_mtd(struct mtd_info *mtd, struct erase_info *instr)
{
	unsigned int addr = instr->addr;
	unsigned int len  = 0;

	/* �жϲ��� */
	if ((addr & (spi_flash_dev.erasesize - 1)) || (instr->len & (spi_flash_dev.erasesize - 1)))
	{
		printk("addr/len is not aligned\n");
		return -EINVAL;
	}

	for (len = 0; len < instr->len; len += 4096)
	{
		spi_flash_erase_sector(addr);
		addr += 4096;
	}
    
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	return 0;
}

static int spi_flash_read_mtd(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf)
{
	spi_flash_read(from, buf, len);
	*retlen = len;
	return 0;
}

static int spi_flash_write_mtd(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf)
{
	unsigned int addr = to;
	unsigned int wlen  = 0;

	/* �жϲ��� */
	if ((to & (spi_flash_dev.writesize - 1)) || (len & (spi_flash_dev.writesize - 1)))
	{
		printk("addr/len is not aligned\n");
		return -EINVAL;
	}

	for (wlen = 0; wlen < len; wlen += 256)
	{
		spi_flash_page_program(addr, (unsigned char *)buf, 256);
		addr += 256;
		buf += 256;
	}

	*retlen = len;
	return 0;
}

/* ���ҵ���Ӧ���ֵ�spi_deviceʱ�������spi_driver��probe���� */
static int __devinit spi_flash_probe(struct spi_device *spi)
{
	unsigned int MID, DID;
	
	spi_flash = spi;

	printk("%s\n", __FUNCTION__);
	
//	spi_tft_rs_pin = (int)spi->dev.platform_data;						/* spi->devָ��spi_board_info����spi_new_device()�ﱻע�� */
	s3c2410_gpio_setpin(spi->chip_select, 1);	
	s3c2410_gpio_cfgpin(spi->chip_select, S3C2410_GPIO_OUTPUT);		/* spi->chip_selectָ��spi_board_info��chip_select����spi_new_device()�ﱻע�� */

	/* ��ȡflash��MID��DID */
	spi_flash_read_id(&MID, &DID);
	printk("MID = %02x\n", MID);	
	printk("DID = %04x\n", DID);

	/* ȥ��status reg��WP���ű��� */
	spi_flash_status_reg_protect_off();

	/* ȥ���洢�ռ�д���� */
	spi_flash_chip_protect_off();

//	printk("spi_tft_rs_pin = %08x\n", spi_tft_rs_pin);					/* 0xa7 == S3C2410_GPF7 */
//	printk("spi->chip_select = %08x\n", spi->chip_select);					/* 0xea == S3C2410_GPH10 */


	/* ����mtd_info */
	memset(&spi_flash_dev, 0, sizeof(spi_flash_dev));

	/* Setup the MTD structure */
	spi_flash_dev.name = "spigpio_flash_mtd";
	spi_flash_dev.type = MTD_NORFLASH;
	spi_flash_dev.flags = MTD_CAP_NORFLASH;
	spi_flash_dev.size = 0x200000;  	/* 2M */
	spi_flash_dev.writesize = 256;
	spi_flash_dev.erasesize = 4096;  	/* ��������С��λ */
	spi_flash_dev.priv = NULL;			/* ˽������ */

	spi_flash_dev.owner = THIS_MODULE;
	spi_flash_dev.erase = spi_flash_erase_mtd;
	spi_flash_dev.read = spi_flash_read_mtd;
	spi_flash_dev.write = spi_flash_write_mtd;

	/* ע��mtd_info */
	add_mtd_device(&spi_flash_dev);	
	
	return 0;
}


static int __devexit spi_flash_remove(struct spi_device *spi)
{
	del_mtd_device(&spi_flash_dev);
	return 0;
}

static struct spi_driver spi_flash_drv = {
	.driver = {
		.name	= "spigpio_flash",
		.owner	= THIS_MODULE,
	},
	.probe		= spi_flash_probe,
	.remove		= __devexit_p(spi_flash_remove),
};

static int spi_flash_init(void)
{
    return spi_register_driver(&spi_flash_drv);
}

static void spi_flash_exit(void)
{
    spi_unregister_driver(&spi_flash_drv);
}

module_init(spi_flash_init);
module_exit(spi_flash_exit);
MODULE_DESCRIPTION("OLED SPI Driver");
MODULE_AUTHOR("weidongshan@qq.com,www.100ask.net");
MODULE_LICENSE("GPL");
