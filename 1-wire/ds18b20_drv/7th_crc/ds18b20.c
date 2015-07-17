/****************************************************************************************************************
 * �ļ�����	��	DS18B20_drive.c
 * ���		:	OK6410 DS18B20����
 * ����		��	����Ԫ��cp1300@139.com��
 * ����ʱ��	��	2012/09/18 20��37
 * �޸�ʱ��	��	2012/09/18
 * ˵��		��	OK6410 �����壨S3C6410��DS18B20��GPIO������
 ****************************************************************************************************************/

//ϵͳͷ�ļ�
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/poll.h>

/* linux/interrupt.h�а���request_irq(),free_irq()������ */
#include <linux/interrupt.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/mach/irq.h>

#include <asm/arch/regs-irq.h>
#include <asm/arch/regs-gpio.h>

#include <asm/plat-s3c24xx/cpu.h>
//#include <asm/plat-s3c24xx/pm.h>
#include <asm/plat-s3c24xx/irq.h>

#include <linux/device.h>

#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/sysdev.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/miscdevice.h>

#define DS18B20_RESET_ERROR		-1	//��ʼ��ʧ��
#define DS18B20_CRC_ERROR			-2	//CRC����

//����ģ������
#define DEVICE_NAME "DS18B20"
//ϵͳUS��ʱ����
#define Delay_US(x)		udelay(x)
#define DS18B20_PIN		S3C2443_GPH14

//DS18B20�豸�����ź���
struct semaphore DS18B20_sem;
static struct class *class;
static struct class_device *class_dev;
int major;


//��������
///////////////////////////////////////////////
static long OK6410_DS18B20_ioctl(
		struct file *file,
		unsigned int cmd,
		unsigned long arg);
static ssize_t OK6410_DS18B20_write(
		struct file *file,
		const char __user *buff,
		size_t size,
		loff_t *loff);
static ssize_t OK6410_DS18B20_read(
		struct file *file,
		char __user *buff,
		size_t size,
		loff_t *loff);
///////////////////////////////////////////////////

unsigned char CalculateCRCValue(unsigned char ucNewData,unsigned char ucCRCValue)
{
	int bXorValue;	//
	unsigned char ucTempValue;		//ģ��CRCӲ���ڲ���8λ���ݼĴ���unsigned charucCount;
	unsigned char ucCount;
	for(ucCount = 0;ucCount < 8;ucCount++)
	{
		//��ȡ������λ��CRC�Ĵ��������ݵ����ֵ��D0λ��Ϊ�ڲ�ͨ��������
		bXorValue = (ucNewData ^ ucCRCValue) & 0x01;//ִ��X4��X5��������
		if(bXorValue)
		{
			ucTempValue = ucCRCValue^0x18;
		}
		else
		{
			ucTempValue = ucCRCValue;
		}
		//CRC�Ĵ����޽�λ����
		ucTempValue >>= 1;
		//���Ͻ�λλ����D0λ
		if(bXorValue)
		{
			ucCRCValue = ucTempValue | 0x80;
		}
		else
		{
			ucCRCValue = ucTempValue;
		}

		//Ϊ������һλ��׼��
		ucNewData >>= 1;
	}
	return ucCRCValue;
}

//��ڲ�������У������ݻ����������ݳ���//�������ݣ�ȡ��ʢ��EPS11��������
//0x280xB40xC10xE90x000x000x000xF0
//DS18B2064bitRomCode
//��Ӧ��У����Ϊ:0xE1 0xE4 0x1C 0x4B 0x66 0xB8 0xF0 0x00
//DS18B20�ļ������Ϊ0x28���ټ���CRCУ�飬ȷ����дͨѶ�ɿ���
int DS18B20_CheckCRC8(unsigned char*ucDataBuff,unsigned char ucDataLength)
{
	unsigned char ucCRCValue;
	unsigned char ucCount;
	
	ucCRCValue = 0;
	for(ucCount = 0;ucCount < ucDataLength; ucCount++)
	{
		ucCRCValue = CalculateCRCValue(ucDataBuff[ucCount],ucCRCValue);
	}
	//У����ȷ
	if(ucCRCValue == 0)
	{
		return 0;
	}
	//У�����
	else
	{
		return -1;
	}
}


//����DS18B20 IOΪ�������ģʽ
void Set18b20IOout(void)
{
#if 0
	unsigned int reg;

	reg = readl(S3C64XX_GPECON);
	reg |= 1;
	writel(reg,S3C64XX_GPECON);
#endif
	s3c2410_gpio_cfgpin(DS18B20_PIN, S3C2410_GPIO_OUTPUT);
}


//��IO
unsigned char Read18b20IO(void)
{
#if 0
	unsigned int reg;

	reg = readl(S3C64XX_GPEDAT);
	if(reg & 1)
		return 1;
	else
		return 0;
#endif
	if(s3c2410_gpio_getpin(DS18B20_PIN))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


//дIO
void Write18b20IO(unsigned char data)
{
#if 0
	unsigned int reg;

	reg = readl(S3C64XX_GPEDAT);
	if(data)
		reg |= 1;
	else
		reg &= ~1;
	writel(reg,S3C64XX_GPEDAT);
#endif
	s3c2410_gpio_setpin(DS18B20_PIN, data);
}


//����DS18B20 IO����ģʽ
void Set18b20IOin(void)
{
#if 0
	unsigned int reg;

	reg = readl(S3C64XX_GPECON);
	reg &= (~0xf);
	writel(reg,S3C64XX_GPECON);
#endif

	s3c2410_gpio_cfgpin(DS18B20_PIN, S3C2410_GPIO_INPUT);
}


//��λDS18B20
u8 DS18B20_Reset(void)
{
	u8 i = 0;

	// ����GPIOB0���ģʽ
	Set18b20IOout();
	// ��18B20����һ�������أ������ָߵ�ƽ״̬Լ100΢�� 
	Write18b20IO(1);
	Delay_US(100);
	// ��18B20����һ���½��أ������ֵ͵�ƽ״̬Լ600΢�� 
	Write18b20IO(0);
	Delay_US(600);
	// ��18B20����һ�������أ���ʱ���ͷ�DS18B20���� 
	Write18b20IO(1);
	// ���϶����Ǹ�DS18B20һ����λ���� 
	// ͨ���ٴ�����GPIOB1���ų�����״̬�����Լ�⵽DS18B20�Ƿ�λ�ɹ�
	Set18b20IOin();
	Delay_US(15);
	// ���������ͷź�����״̬Ϊ�ߵ�ƽ����λʧ��
	while(Read18b20IO())
	{
		i ++;
		Delay_US(10);
		if(i > 30)
			return 1;
	}
	Delay_US(100);
	return 0x00;
}


//��DS18B20����
u8 DS18B20_ReadData(void)
{
	u8 i,data = 0;

	Delay_US(5);
	for(i = 0;i < 8;i ++)
	{
		data >>= 1;
		Set18b20IOout();
		Write18b20IO(1);
		Delay_US(2);
		//��ʼread slot
		Write18b20IO(0);
		Delay_US(3);
		Write18b20IO(1);
		Set18b20IOin();	
		Delay_US(2);

		if(Read18b20IO())
			data |= 0x80;
		Delay_US(60);
	}
	Delay_US(5);
	return data;
}



//дDS18B20����
void DS18B20_WriteData(u8 data)
{
	u8 i;

	Delay_US(5);
	Set18b20IOout();
	for(i = 0;i < 8;i ++)
	{
		//��ʼwrite slot
		Write18b20IO(0);
		Delay_US(3);
		Write18b20IO(data & 0x01);
		Delay_US(80);
		Write18b20IO(1);
		data >>= 1;
		Delay_US(5);
	}
	Delay_US(5);
}

//��ȡ�¶�
int DS18B20_ReadTemper(int *data)
{
	u8 th, tl;
	unsigned char data_read[9] = {0};
	int i;

	if(DS18B20_Reset())
	{
		return DS18B20_RESET_ERROR;
	}
	Delay_US(50);
	DS18B20_WriteData(0xcc);
	DS18B20_WriteData(0x44);
	
	if(DS18B20_Reset())
	{
		return DS18B20_RESET_ERROR;
	}
	Delay_US(50);
	DS18B20_WriteData(0xcc);
	DS18B20_WriteData(0xbe);

	Delay_US(50);

	for(i = 0; i < 9; i++)
	{
		data_read[i] = DS18B20_ReadData();
	}

	/* CRC ERROR */
	if(DS18B20_CheckCRC8(data_read, 9))
	{
		printk("crc error\n");
		for(i = 0; i < 9; i++)
		{
			printk("data[%d] = 0x%02x\n", i, data_read[i]);
		}
		return DS18B20_CRC_ERROR;
	}
	
	if(DS18B20_Reset())
	{
		return DS18B20_RESET_ERROR;
	}

	tl = data_read[0];
	th = data_read[1];
	
	*data = th;
	*data <<= 8;
	*data |= tl;
	*data = *data * 625 / 100;

	return 0;
}


/****************************************************************************************************************
*������		:	static long OK6410_DS18B20_ioctl(
						struct file *file,
						unsigned int cmd,
						unsigned long arg)
*����       :	���������DS18B20����ģ�飬��ʵ�����ã�ֱ�ӷ���0
*����       :	������
*����       :	0
*����       : 	��
*����       :	����Ԫ��cp1300@139.com��
*����ʱ��	:	2012/09/18 20��38
*����޸�ʱ��:	2012/09/18 20��38
*˵��		:	��
****************************************************************************************************************/
static long OK6410_DS18B20_ioctl(
		struct file *file,
		unsigned int cmd,
		unsigned long arg)
{
	return 0;
}


/****************************************************************************************************************
*������		:	static ssize_t OK6410_DS18B20_write(
						struct file *file,
						const char __user *buff,
						size_t size,
						loff_t *loff)
*����       :	д���ݵ�DS18B20����ģ�飬��Ч����������0
*����       :	file���ļ�ָ�루�����ã���buff�����ݻ�����ָ�룻buff������������loff:������
*����       :	0���ɹ�;<0:ʧ��
*����       : 	linux�ײ��
*����       :	����Ԫ��cp1300@139.com��
*����ʱ��	:	2012/09/18 20��38
*����޸�ʱ��:	2012/09/18 20��38
*˵��		:	д��Ч
****************************************************************************************************************/
static ssize_t OK6410_DS18B20_write(
		struct file *file,
		const char __user *buff,
		size_t size,
		loff_t *loff)
{

	return 0;
}


/****************************************************************************************************************
*������		:	static ssize_t OK6410_DS18B20_read(
						struct file *file,
						char __user *buff,
						size_t size,
						loff_t *loff)
*����       :	��DS18B20״̬���͵�ƽ����
*����       :	file���ļ�ָ�루�����ã���buff�����ݻ�����ָ�룻buff������������loff:������
*����       :	0���ɹ�;<0:ʧ��
*����       : 	linux�ײ��
*����       :	����Ԫ��cp1300@139.com��
*����ʱ��	:	2012/09/18 20��38
*����޸�ʱ��:	2012/09/18 20��38
*˵��		:	��ȡ������һ��16λ�з��ŵ��¶�
****************************************************************************************************************/
static ssize_t OK6410_DS18B20_read(
		struct file *file,
		char __user *buff,
		size_t size,
		loff_t *loff)
{
	int temp;
	int ret;

	if(down_interruptible(&DS18B20_sem))		//��ȡ�ź���
		return -ERESTARTSYS;
	ret = DS18B20_ReadTemper(&temp);			//��ȡ�¶�
	if(ret)				//DS18B20��ʼ��ʧ��
	{
		up(&DS18B20_sem);							//�ͷ��ź���
		if(ret == DS18B20_RESET_ERROR)
		{
			printk("reset error\n");
		}
		return ret;							//DS18B20��ȡʧ�ܣ����ش���
	}

	ret = copy_to_user(buff, &temp, size);			//���¶�д���û��ռ�							

	up(&DS18B20_sem);							//�ͷ��ź���

	return 0;
}


/*	����ṹ���ַ��豸�����ĺ���
*	��Ӧ�ó�������豸�ļ����ṩ��open,read,write�Ⱥ�����
*	���ջ���õ�����ṹ�еĶ�Ӧ����
*/
static struct file_operations dev_fops = {
		.owner				= THIS_MODULE,		//����һ���ָ꣬�����ģ��ʱ�Զ�������__this_module����
		.unlocked_ioctl 		= OK6410_DS18B20_ioctl,
		.read				= OK6410_DS18B20_read,
		.write				= OK6410_DS18B20_write,
};

/****************************************************************************************************************
*������		:	static int  __init OK6410_DS18B20_init(void)
*����       :	DS18B20ģ���ʼ������
*����       :	��
*����       :	0���ɹ���<0:ʧ��
*����       : 	linux�ײ�궨��
*����       :	����Ԫ��cp1300@139.com��
*����ʱ��	:	2012/09/18 20��38
*����޸�ʱ��:	2012/09/18 20��38
*˵��		:	��ʼ��DS18B20Ӳ����ע��DS18B20����
****************************************************************************************************************/
static int  __init OK6410_DS18B20_init(void)
{
	/* ָ�����豸��ΪBUTTON_MAJOR������0��ʾ�ɹ� */
	major = register_chrdev(0, DEVICE_NAME, &dev_fops);
	
	class = class_create(THIS_MODULE, DEVICE_NAME);
	class_dev = class_device_create(class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); /* /dev/buttons */

	init_MUTEX(&DS18B20_sem);			//ע���ź���
	printk(DEVICE_NAME " initialized\n");
	return 0;							//���سɹ�
}

/****************************************************************************************************************
*������		:	static void __exit OK6410_DS18B20_exit(void)
*����       :	ж��DS18B20����
*����       :	��
*����       :	��
*����       : 	linux�ײ��
*����       :	����Ԫ��cp1300@139.com��
*����ʱ��	:	2012/09/18 20��38
*����޸�ʱ��:	2012/09/18 20��38
*˵��		:	ж������
****************************************************************************************************************/
static void __exit OK6410_DS18B20_exit(void)
{
	unregister_chrdev(major, DEVICE_NAME);
	class_device_unregister(class_dev);
//	class_device_destroy(sixthdrv_class, MKDEV(major, 0));
	class_destroy(class);
}



//��̬���������ӿڣ����룩
module_init(OK6410_DS18B20_init);
module_exit(OK6410_DS18B20_exit);
//������Ϣ���Ǳ��裩
MODULE_AUTHOR("sdwuyawen@126.com");						//������������
MODULE_DESCRIPTION("OK6410(S3C6410) DS18B20 Driver");	//һЩ������Ϣ
MODULE_LICENSE("GPL");	//��ѭ��Э��




