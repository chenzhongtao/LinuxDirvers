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
#include <asm/plat-s3c24xx/pm.h>
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
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>


static struct class *sixthdrv_class;
static struct class_device	*sixthdrv_class_dev;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdat;
volatile unsigned long *gpfudp;

/* ��ʱ�� */
static struct timer_list buttons_timer;


static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static struct work_struct my_wq; 					/* ����һ���������� */
static void my_wq_func(struct work_struct *work);	/* ����һ�������� */

/* �������tasklet���� */
static void my_tasklet_func(unsigned long t);
DECLARE_TASKLET(my_tasklet, my_tasklet_func, 0);

/* �ж��¼���־, �жϷ����������1��sixth_drv_read������0 */
static volatile int ev_press = 0;

static struct fasync_struct *button_async;


struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};


/* ��ֵ: ����ʱ, 0x01, 0x02, 0x03, 0x04, 0x05 */
/* ��ֵ: �ɿ�ʱ, 0x81, 0x82, 0x83, 0x84, 0x85 */
static unsigned char key_val;

struct pin_desc pins_desc[5] = {
	{S3C2410_GPF0, 0x01},
	{S3C2410_GPF1, 0x02},
	{S3C2410_GPF2, 0x03},
	{S3C2410_GPF3, 0x04},
	{S3C2410_GPF4, 0x05},
};

static struct pin_desc *irq_pd = 0;

//static atomic_t canopen = ATOMIC_INIT(1);	/* ����ԭ�ӱ�������ʼ��Ϊ1 */
static DECLARE_MUTEX(button_lock);		/* �����ź��� */

/*
  * ȷ������ֵ
  */
static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	/* 10ms��������ʱ�� */
	irq_pd = (struct pin_desc *)dev_id;
	mod_timer(&buttons_timer, jiffies+HZ/100);
		
	return IRQ_RETVAL(IRQ_HANDLED);
}

/* �������к��� */
static void my_wq_func(struct work_struct *work)
{
	(void)work;
	printk("my_wq_func before sleep\n");
	msleep(1000);
	printk("my_wq_func after sleep\n");
}

/* tasklet���� */
void my_tasklet_func(unsigned long t)
{
	(void)t;
	printk("tasklet is executing\n");
}

static int sixth_drv_open(struct inode *inode, struct file *file)
{
	unsigned long pend;
	
	/* �������� */
	/*if(!atomic_dec_and_test(&canopen))
	{
		atomic_inc(&canopen);
		return -EBUSY;
	}*/

	/* ��ȡ�ź��� */
	if (file->f_flags & O_NONBLOCK)
	{
		if (down_trylock(&button_lock))
			return -EBUSY;
	}
	else
	{
		/* ��ȡ�ź��� */
		down(&button_lock);
	}
	
	/* �����ڲ����� */
	*gpfudp &= ~((0x3<<(0*2)) | (0x3<<(1*2)) | (0x3<<(2*2)) | (0x3<<(3*2)) | (0x3<<(4*2)));
	*gpfudp |= ((0x2<<(0*2)) | (0x2<<(1*2)) | (0x2<<(2*2)) | (0x2<<(3*2)) | (0x2<<(4*2)));

	/* ������EINT3���һ���������жϣ�������open�����EINT0-EINT3��SRCPND */
	pend = __raw_readl(S3C2410_SRCPND);
	printk("S3C2410_SRCPND=0x%08x\n", (unsigned int)pend);
	
	__raw_writel(pend & 0x0f, S3C2410_SRCPND);

	pend = __raw_readl(S3C2410_SRCPND);
	printk("S3C2410_SRCPND=0x%08x\n", (unsigned int)pend);
	
	/* ����GPF0,1,2,3,4Ϊ�������� */
	request_irq(IRQ_EINT0,  buttons_irq, IRQT_BOTHEDGE, "S1", &pins_desc[0]);
	request_irq(IRQ_EINT1,  buttons_irq, IRQT_BOTHEDGE, "S2", &pins_desc[1]);
	request_irq(IRQ_EINT2,  buttons_irq, IRQT_BOTHEDGE, "S3", &pins_desc[2]);
	request_irq(IRQ_EINT3,  buttons_irq, IRQT_BOTHEDGE, "S4", &pins_desc[3]);	
	request_irq(IRQ_EINT4,  buttons_irq, IRQT_BOTHEDGE, "S5", &pins_desc[4]);	

	return 0;
}

ssize_t sixth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK)
	{
		if (!ev_press)
			return -EAGAIN;
	}
	else
	{
		/* ���û�а�������, ���� */
		/* �ڶ���������0��ʾ�������ߣ���0������ */
		wait_event_interruptible(button_waitq, ev_press);
	}
//	wait_event_interruptible(button_waitq, ev_press);

	/* ����а�������, ���ؼ�ֵ */
	copy_to_user(buf, &key_val, 1);
	ev_press = 0;
	
	return 1;
}


int sixth_drv_close(struct inode *inode, struct file *file)
{	
	free_irq(IRQ_EINT0, &pins_desc[0]);
	free_irq(IRQ_EINT1, &pins_desc[1]);
	free_irq(IRQ_EINT2, &pins_desc[2]);
	free_irq(IRQ_EINT3, &pins_desc[3]);
	free_irq(IRQ_EINT4, &pins_desc[4]);

	/* �������� */
//	atomic_inc(&canopen);

	/* �ͷ��ź��� */
	up(&button_lock);
	
	return 0;
}

static unsigned sixth_drv_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait); // �����������ߣ��ѵ�ǰ���̹ҵ�button_waitq����

	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;	/* �������0����ǰ���̻����ߣ����򲻻����� */
}

static int sixth_drv_fasync (int fd, struct file *filp, int on)
{
	printk("driver: sixth_drv_fasync\n");
	/* ��ʼ��button_async�ṹ�壬����kill_fasync (&button_async, SIGIO, POLL_IN)�Ϳ��Է����źŸ���Ӧ������
		fcntl(fd, F_SETOWN, getpid())�Ѿ���filp->f_owner����Ϊ��Ӧ����PID */
	return fasync_helper (fd, filp, on, &button_async);	/* ����on�Ƿ�Ϊ0����ʼ�������ͷ�fasync_struct�ṹ��(&button_async) */
}

static struct file_operations sencod_drv_fops = {
    .owner   =  THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
    .open    =  sixth_drv_open,     
	.read	 =	sixth_drv_read,	   
	.release =  sixth_drv_close,
	.poll    =  sixth_drv_poll,
	.fasync	 =  sixth_drv_fasync,	/* fcntl(fd, F_SETFL, Oflags | FASYNC)ʱ������ */
};

static void buttons_timer_function(unsigned long data)
{
	struct pin_desc * pindesc = irq_pd;
	unsigned int pinval;

	if (!pindesc)	/* ��δ���밴���ж�ʱ���Ѿ������˶�ʱ����ʱ���� */
		return;
	
	pinval = s3c2410_gpio_getpin(pindesc->pin);

	if (pinval)
	{
		/* �ɿ� */
		key_val = 0x80 | pindesc->key_val;
	}
	else
	{
		/* ���� */
		key_val = pindesc->key_val;
	}

	ev_press = 1;                  /* ��ʾ�жϷ����� */
	wake_up_interruptible(&button_waitq);   /* �������ߵĽ��� */

	kill_fasync (&button_async, SIGIO, POLL_IN);	/* ����SIGIO�źŸ�Ӧ�ó���button_asyncȷ�������ĸ����� */

	/* ���ȹ�������ִ�� */
	schedule_work(&my_wq);

	//����taskletִ��
	tasklet_schedule(&my_tasklet);
}


int major;
static int sixth_drv_init(void)
{
	/* ��ʼ����ʱ�� */
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	/* 10ms����붨ʱ���ж� */
	buttons_timer.expires  = jiffies+HZ/100;
	add_timer(&buttons_timer); 

	/* ��ʼ��workqueue */
	INIT_WORK(&my_wq, (void(*) (struct work_struct *))my_wq_func);
	
	major = register_chrdev(0, "sixth_drv", &sencod_drv_fops);

	sixthdrv_class = class_create(THIS_MODULE, "sixth_drv");

	sixthdrv_class_dev = class_device_create(sixthdrv_class, NULL, MKDEV(major, 0), NULL, "buttons"); /* /dev/buttons */
//	sixthdrv_class_dev = class_device_create(sixthdrv_class, NULL, MKDEV(major, 1), NULL, "buttons2"); /* /dev/buttons */
//	sixthdrv_class_dev = class_device_create(sixthdrv_class, NULL, MKDEV(major, 2), NULL, "buttons3"); /* /dev/buttons */
//	sixthdrv_class_dev = class_device_create(sixthdrv_class, NULL, MKDEV(major, 3), NULL, "buttons4"); /* /dev/buttons */

	gpfcon = (volatile unsigned long *)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;
	gpfudp = gpfdat + 1;

	return 0;
}

static void sixth_drv_exit(void)
{
	unregister_chrdev(major, "third_drv");
//	class_device_unregister(sixthdrv_class_dev);
	class_device_destroy(sixthdrv_class, MKDEV(major, 0));
//	class_device_destroy(sixthdrv_class, MKDEV(major, 1));
//	class_device_destroy(sixthdrv_class, MKDEV(major, 2));
//	class_device_destroy(sixthdrv_class, MKDEV(major, 3));
	class_destroy(sixthdrv_class);
	iounmap(gpfcon);
	
//	return 0;
}


module_init(sixth_drv_init);

module_exit(sixth_drv_exit);

MODULE_LICENSE("GPL");

