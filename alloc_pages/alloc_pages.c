#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/highmem.h>

unsigned char *vm_addr;
unsigned char *km_addr;

struct page *page;
unsigned char *v_addr;
unsigned char *m_free_pages;

struct page *pagekmap;
unsigned char *kmapaddr;

static int mem_test_init(void)
{
//	s3c_lcd = framebuffer_alloc(0, NULL);	/* ����0�Ƕ������Ĵ�С������Ϊ0 */

	vm_addr = vmalloc(128);
	printk("vmalloc = %08x\n", (unsigned int)vm_addr);

	km_addr = kmalloc(128, GFP_KERNEL);
	printk("km_addr = %08x\n", (unsigned int)km_addr);	

	/* ��ȡ����ҳ */
	page = alloc_pages(GFP_KERNEL, 2);
	if(!page)
	{
		printk("alloc_pages error\n");
	}
	printk("struct page addr = %08x\n", (unsigned int)page);

	/* ��ȡ�����ַ */
	v_addr = (unsigned char *)page_address(page);
	printk("page v_addr = %08x\n", (unsigned int)v_addr);

	/* �ٻ�ȡһ��ҳ�򣬲���ȡ���������ַ */
	m_free_pages = (unsigned char *)__get_free_pages(GFP_KERNEL, 2);
	if(!m_free_pages)
	{
		printk("__get_free_pages error\n");
	}
	printk("__get_free_pages addr = %08x\n", (unsigned int)m_free_pages);

	/* ��ȡ����ҳ */
	pagekmap = alloc_pages(GFP_KERNEL, 2);
	if(!pagekmap)
	{
		printk("alloc_pages error\n");
	}
	printk("struct page pagekmap addr = %08x\n", (unsigned int)pagekmap);
	/* ��ȡ�����ַ */
	kmapaddr = kmap(pagekmap);
	printk("kmapaddr = %08x\n", (unsigned int)kmapaddr);
	
	return 0;
}

static void mem_test_exit(void)
{
	vfree(vm_addr);
	kfree(km_addr);
	__free_pages(page, 2);
	free_pages((unsigned long)m_free_pages, 2);

	kunmap(pagekmap);
	__free_pages(pagekmap, 2);
	
}

module_init(mem_test_init);
module_exit(mem_test_exit); 

MODULE_LICENSE("GPL");
