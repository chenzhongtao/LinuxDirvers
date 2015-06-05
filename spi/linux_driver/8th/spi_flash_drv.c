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


/* �ο�:
 * drivers/mtd/devices/mtdram.c
 * drivers/mtd/devices/m25p80.c
 */

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


/* ���ҵ���Ӧ���ֵ�spi_deviceʱ�������spi_driver��probe���� */
static int __devinit spi_flash_probe(struct spi_device *spi)
{
	unsigned int MID, DID;
	spi_flash = spi;

	printk("%s\n", __FUNCTION__);
	
//	spi_tft_rs_pin = (int)spi->dev.platform_data;						/* spi->devָ��spi_board_info����spi_new_device()�ﱻע�� */
	s3c2410_gpio_setpin(spi->chip_select, 1);	
	s3c2410_gpio_cfgpin(spi->chip_select, S3C2410_GPIO_OUTPUT);		/* spi->chip_selectָ��spi_board_info��chip_select����spi_new_device()�ﱻע�� */

	spi_flash_read_id(&MID, &DID);

	printk("MID = %02x\n", MID);	
	printk("DID = %04x\n", DID);

	printk("%s\n", __FUNCTION__);
	
//	printk("spi_tft_rs_pin = %08x\n", spi_tft_rs_pin);					/* 0xa7 == S3C2410_GPF7 */
//	printk("spi->chip_select = %08x\n", spi->chip_select);					/* 0xea == S3C2410_GPH10 */
	
    return 0;
}


static int __devexit spi_flash_remove(struct spi_device *spi)
{
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
