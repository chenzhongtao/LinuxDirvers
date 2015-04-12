/* �ο� 
 * drivers\mtd\nand\s3c2410.c
 * drivers\mtd\nand\at91_nand.c
 */


static struct nand_chip *s3c_nand;
static struct mtd_info *s3c_mtd;

static void s3c2416_select_chip(struct mtd_info *mtd, int chipnr)
{
	if (chipnr == -1)
	{
		/* ȡ��ѡ��: NFCONT[1]��Ϊ0 */
	}
	else
	{
		/* ѡ��: NFCONT[1]��Ϊ1 */
		
	}
}

static void s3c2416_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (ctrl & NAND_CLE)
	{
		/* ������: NFCMMD=dat */
	}
	else
	{
		/* ����ַ: NFADDR=dat */
	}
}

/*
 * ����1��ʾready,0��ʾbusy
 */
int s3c2416_dev_ready(struct mtd_info *mtd)
{
	return "NFSTAT��bit[0]";
}

static int s3c_nand_init(void)
{
	/* 1. ����һ��nand_chip�ṹ�� */
	s3c_nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);
	
	/* 2. ����nand_chip */
	/* ����nand_chip�Ǹ�nand_scan����ʹ�õ�, �����֪����ô����, �ȿ�nand_scan��ôʹ�� 
	 * ��Ӧ���ṩ:ѡ��,������,����ַ,������,������,�ж�״̬�Ĺ���
	 */
	s3c_nand->select_chip = s3c2416_select_chip;
	s3c_nand->cmd_ctrl = s3c2416_cmd_ctrl;
	s3c_nand->IO_ADDR_R = "NFDATA�������ַ";
	s3c_nand->IO_ADDR_W = "NFDATA�������ַ";
	s3c_nand->dev_ready = s3c2416_dev_ready;
	/* 3. Ӳ����ص����� */

	/* 4. ʹ��: nand_scan */
	s3c_mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	
	nand_chip->priv = s3c_mtd;		/* link the private data structures */
	s3c_mtd->priv = s3c_nand;
	s3c_mtd->owner = THIS_MODULE;
	
	nand_scan(s3c_mtd, 1);		/* ʶ��NAND FLASH, ����mtd_info */
	
	/* 5. add_mtd_partitions */
}


static void s3c_nand_exit(void)
{

}

module_init(s3c_nand_init);
module_exit(s3c_nand_exit);

MODULE_LICENSE("GPL");