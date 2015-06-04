///////////////////////////////////////////////////////////////
// ������ֻ��ѧϰʹ�ã�δ������˾��ɣ��������������κ���ҵ��;
// ���ÿ������ͺ�:Tiny2416��Mini2451��Tiny2451
// ������̳:www.arm9home.net
// �޸�����:2013/7/1
// ��Ȩ���У�����ؾ���
// Copyright(C) ��������֮�ۼ�����Ƽ����޹�˾
// All rights reserved							
///////////////////////////////////////////////////////////////

#define REGISTER_USE_CACHE    0
#define MMU_BASE  			  0x31000000					 // ҳ���ַ
#define MMU_FULL_ACCESS       (3 << 10)  					 // ����Ȩ��  
#define MMU_DOMAIN            (0 << 5)   					 // �����ĸ���  
#define MMU_SPECIAL           (1 << 4)   					 // bit 4������1  
#define MMU_CACHEABLE         (1 << 3)   					 // ����ʹ��cache  
#define MMU_BUFFERABLE        (1 << 2)   					 // ����ʹ��write buffer  
#define MMU_SECDESC           (2)        					 // ��ʾ���Ƕ�������  
#define ENABLE_DCACHE_ICACHE  1
#define MMU_SECDESC_WB        (MMU_FULL_ACCESS | MMU_DOMAIN | \
	                          MMU_SPECIAL | MMU_CACHEABLE | \
	                          MMU_BUFFERABLE | MMU_SECDESC)
#define MMU_SECDESC_NCNB      (MMU_FULL_ACCESS | MMU_DOMAIN | \
	                          MMU_SPECIAL | MMU_SECDESC)

// ��ʼ��mmu
void mmu_init(void)
{
	int i;
	volatile unsigned long *table = (volatile unsigned long *)MMU_BASE;

	// ����ҳ����
	for(i=0; i<=0x102; i++)									 // �Ĵ���
	{
		table[0x4a0+i] = (0x4a000000 + i * 0x100000) | MMU_SECDESC_NCNB;
	}

	for(i=0; i<0x40; i++)										 // �ڴ�
	{
		table[0x300+i] = (0x30000000 + i * 0x100000) | MMU_SECDESC_WB;
	}
//	table[0x400] = 0x40000000 | MMU_SECDESC_WB;  		   	 // MMUʹ��ǰ��Ĵ������ڵĿռ�, �������ǵ������ַ���������ַ 
//	table[0x0]   = 0x40000000 | MMU_SECDESC_WB;  			 // �ж�������ӳ�䵽0��ַ

	/* �����ӵ� */
	table[0x000]   = 0x00000000 | MMU_SECDESC_WB;  	//ӳ��1M,0x100000
	table[0xC00]   = 0x30000000 | MMU_SECDESC_WB;  
	/* �����ӵ� ���� */

	// ����mmu
	__asm__ (

		"mov    r1, #0\n"
		"mcr    p15, 0, r1, c7, c7, 0\n"    				 // ʹ��ЧICaches��DCaches  

		"mcr    p15, 0, r1, c7, c10, 4\n"   				 // drain write buffer on v4  
		"mcr    p15, 0, r1, c8, c7, 0\n"    				 // ʹ��Чָ�����TLB  


		"mcr p15, 0, %0, c2, c0, 0\n"						 // write TTB register  
		"mrc p15, 0, r1, c3, c0, 0\n" 						 // read domain 15:0 access permissions  
		"orr r1, r1, #3\n"            						 // domain 0, Accesses are not checked  
		"mcr p15, 0, r1, c3, c0, 0\n" 						 // write domain 15:0 access permissions  

		"mrc p15, 0, r1, c1, c0, 0\n" 						 // Read control register  

#if ENABLE_DCACHE_ICACHE
		"orr r1, r1, #(1<<2)\n"       						 // Data cache enable  
		"orr r1, r1, #(1<<12)\n"      						 // Instruction cache enable  
		"orr r1, r1, #(1<<14)\n"      						 // Round robin replacement  
#endif
		"orr r1, r1, #(1<<0)\n"       						 // MMU enable  

		"mcr p15,0,r1,c1, c0,0\n"     						 // write control register  
		:
		: "r" (table)
		: "r1"
	);
}

