#include "s3c2416.h"
#include "gpio_spi.h"

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

#if 0
static void delay_ms(unsigned int ms)
{
	volatile unsigned long i,j;
	for(i = 0; i < ms; i++)
	{
		for(j = 0; j < 0x1000; j++)
		{
		
		}
	}
}
#endif


static void spi_flash_cs_clr(void)
{
	GPHDAT &= ~(1 << 10);
}

static void spi_flash_cs_set(void)
{
	GPHDAT |= (1 << 10);
}

static void spi_flash_send_address(unsigned int addr)
{
	SPIvSendByte((addr >> 16) & 0xFF);
	SPIvSendByte((addr >> 8) & 0xFF);
	SPIvSendByte((addr >> 0) & 0xFF);
}

void spi_flash_read_id(unsigned int *pMID, unsigned int *pDID)
{
	spi_flash_cs_clr();

	SPIvSendByte(CMD_RDID);
//	spi_flash_send_address(0x00);

	*pMID = SPIvSendByte(0x00);

	*pDID = SPIvSendByte(0x00);
	
	*pDID = (*pDID << 8) | SPIvSendByte(0x00);
	
	spi_flash_cs_set();
}

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
	spi_flash_cs_clr();									/* ʹ��Ƭѡ */
	SPIvSendByte(CMD_WREN);						/* �������� */
	spi_flash_cs_set();								/* ����Ƭѡ */
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
	spi_flash_cs_clr();									/* ʹ��Ƭѡ */
	SPIvSendByte(CMD_WRDISEN);						/* �������� */
	spi_flash_cs_set();								/* ����Ƭѡ */
}


static unsigned char spi_flash_read_status_reg1(void)
{
	unsigned char byte;
	
	spi_flash_cs_clr();

	SPIvSendByte(CMD_RDSR1);

	byte = SPIvSendByte(0x00);
	
	spi_flash_cs_set();							/* ����Ƭѡ */

	return byte;
}

static unsigned char spi_flash_read_status_reg2(void)
{
	unsigned char byte;
	
	spi_flash_cs_clr();

	SPIvSendByte(CMD_RDSR2);

	byte = SPIvSendByte(0x00);
	
	spi_flash_cs_set();							/* ����Ƭѡ */

	return byte;
}

static void spi_flash_wait_for_busy(void)
{
	while((spi_flash_read_status_reg1() & 0x01) != 0);
}
static void spi_flash_write_status_reg(unsigned char reg1, unsigned char reg2)
{
	/* д��Write enableָ�����д��status reg */
	spi_flash_write_enable();
	
	spi_flash_cs_clr();

	SPIvSendByte(CMD_WRSR);

	SPIvSendByte(reg1);
	SPIvSendByte(reg2);
	
	spi_flash_cs_set();							/* ����Ƭѡ */

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
	spi_flash_write_enable();

	spi_flash_cs_clr();

	SPIvSendByte(CMD_SE);

	spi_flash_send_address(addr);
	
	spi_flash_cs_set();							/* ����Ƭѡ */

	spi_flash_wait_for_busy();
}

void spi_flash_page_program(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int i;
	
	spi_flash_write_enable();

	spi_flash_cs_clr();

	SPIvSendByte(CMD_PAGE_PROGRAM);

	spi_flash_send_address(addr);

	for(i = 0; i < len; i++)
	{
		SPIvSendByte(buf[i]);
	}
	
	spi_flash_cs_set();							/* ����Ƭѡ */

	spi_flash_wait_for_busy();
}

void spi_flash_read(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int i;
	
	spi_flash_cs_clr();

	SPIvSendByte(CMD_READ);

	spi_flash_send_address(addr);

	for(i = 0; i < len; i++)
	{
		buf[i] = SPIvSendByte(0x00);
	}
	
	spi_flash_cs_set();							/* ����Ƭѡ */	
}

void spi_flash_init(void)
{
	spi_init();
	
	/* deselect flash */
	spi_flash_cs_set();

	/* ȥ��status reg��WP���ű��� */
	spi_flash_status_reg_protect_off();

	/* ȥ���洢�ռ�д���� */
	spi_flash_chip_protect_off();
}