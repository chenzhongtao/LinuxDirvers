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

/* ����/����/ע��һ��platform_driver */

static int led_probe(struct platform_device *pdev)
{
	/* ����platform_device����Դ����ioremap */

	/* ע���ַ��豸�������� */

	printk("led_probe, found led\n");

	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	/* ж���ַ��豸�������� */
	
	/* iounmap */

	printk("led_remove, remove led\n");

	return 0;
}

struct platform_driver led_drv = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver		= {
	.name	= "myled",
	}
};


static int led_drv_init(void)
{
	platform_driver_register(&led_drv);
	return 0;
}

static void led_drv_exit(void)
{
	platform_driver_unregister(&led_drv);
}

module_init(led_drv_init);
module_exit(led_drv_exit);

MODULE_LICENSE("GPL");
