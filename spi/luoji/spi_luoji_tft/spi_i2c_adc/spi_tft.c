#include "s3c2416.h"
#include "spi_tft.h"
#include "gpio_spi.h"


/***************************************************************************************
STM32����ƽ̨����:
�����壺����ԭ��MiniSTM32������
MCU ��STM32_F103_RBT6
���� ��12MHZ
��Ƶ ��72MHZ
����˵��:
//-------------------------------------------------------------------------------------
#define LCD_CTRL   	  	GPIOA		//����TFT���ݶ˿�
#define LCD_LED        	�Ӹߵ�ƽ    
#define LCD_RS         	GPIO_Pin_3	//PB10������TFT --RS
#define LCD_CS        	GPIO_Pin_4 //PB11 ������TFT --CS
#define LCD_RST     	�ӵ�Ƭ����λ��
#define LCD_SCL        	GPIO_Pin_5	//PB13������TFT -- CLK
#define LCD_SDA        	GPIO_Pin_7	//PB15������TFT - SDI
//VCC:���Խ�5VҲ���Խ�3.3V
//LED:���Խ�5VҲ���Խ�3.3V����ʹ���������IO����(�ߵ�ƽʹ��)
//GND���ӵ�Դ��
//˵��������Ҫ��������ռ��IO�����Խ�LCD_CS�ӵأ�LCD_LED��3.3V��LCD_RST������Ƭ����λ�ˣ�
//�������ͷ�3������IO
//�ӿڶ�����Lcd_Driver.h�ڶ��壬
//������IO�ӷ������������ʵ�ʽ����޸���ӦIO��ʼ��LCD_GPIO_Init()
//-----------------------------------------------------------------------------------------
���̹���˵����
1.	��ˢ������
2.	Ӣ����ʾ����ʾ��
3.	������ʾ����ʾ��
4.	�����������ʾʾ��
5.	ͼƬ��ʾʾ��
6.	2D�����˵�ʾ��
7.	������֧�ֺ���/�����л�(������USE_HORIZONTAL,���Lcd_Driver.h)
8.	������֧�����ģ��SPI/Ӳ��SPI�л�(������USE_HARDWARE_SPI,���Lcd_Driver.h)
**********************************************************************************************/


//---------------------------------function----------------------------------------------------//

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

static void lcd_cs_clr(void)
{
	GPHDAT &= ~(1 << 10);
}

static void lcd_cs_set(void)
{
	GPHDAT |= (1 << 10);
}

static void lcd_reset_clr(void)
{
	GPGDAT &= ~(1 << 5);
}

static void lcd_reset_set(void)
{
	GPGDAT |= (1 << 5);
}

static void lcd_rs_clr(void)
{
	GPFDAT &= ~(1 << 7);
}

static void lcd_rs_set(void)
{
	GPFDAT |= (1 << 7);
}

/****************************************************************************
* ��    �ƣ�Lcd_WriteIndex(unsigned char Index)
* ��    �ܣ���Һ����дһ��8λָ��
* ��ڲ�����Index   �Ĵ�����ַ
* ���ڲ�������
* ˵    ��������ǰ����ѡ�п��������ڲ�����
****************************************************************************/
void Lcd_WriteIndex(unsigned char index)
{
	lcd_cs_clr();
	lcd_rs_clr();

	SPIvSendByte(index);

	lcd_cs_set();
}

/****************************************************************************
* ��    �ƣ�Lcd_WriteData(unsigned char Data)
* ��    �ܣ���Һ����дһ��8λ����
* ��ڲ�����dat     �Ĵ�������
* ���ڲ�������
* ˵    �����������ָ����ַд�����ݣ��ڲ�����
****************************************************************************/
void Lcd_WriteData(unsigned char data)
{
	lcd_cs_clr();
	lcd_rs_set();

	SPIvSendByte(data);

	lcd_cs_set();
}

/****************************************************************************
* ��    �ƣ�void LCD_WriteReg(unsigned char Index,unsigned short Data)
* ��    �ܣ�д�Ĵ�������
* ��ڲ�����Index,Data
* ���ڲ�������
* ˵    ����������Ϊ��Ϻ�������Index��ַ�ļĴ���д��Dataֵ
****************************************************************************/
void LCD_WriteReg(unsigned char index,unsigned short data)
{
	Lcd_WriteIndex(index);
  	Lcd_WriteData_16Bit(data);
}

/****************************************************************************
* ��    �ƣ�void Lcd_WriteData_16Bit(unsigned short Data)
* ��    �ܣ���Һ����дһ��16λ����
* ��ڲ�����Data
* ���ڲ�������
* ˵    �����������ָ����ַд��һ��16λ����
****************************************************************************/
void Lcd_WriteData_16Bit(unsigned short data)
{	
	Lcd_WriteData(data >> 8);
	Lcd_WriteData(data);	
}


/****************************************************************************
* ��    �ƣ�void Lcd_Reset(void)
* ��    �ܣ�Һ��Ӳ��λ����
* ��ڲ�������
* ���ڲ�������
* ˵    ����Һ����ʼ��ǰ��ִ��һ�θ�λ����
****************************************************************************/
void Lcd_Reset(void)
{
	lcd_reset_clr();
	delay_ms(100);
	lcd_reset_set();
	delay_ms(50);
}



/*************************************************
��������LCD_Set_XY
���ܣ�����lcd��ʾ��ʼ��
��ڲ�����xy����
����ֵ����
*************************************************/
void Lcd_SetXY(unsigned short Xpos, unsigned short Ypos)
{	
#if USE_HORIZONTAL//��������˺���  	    	
	LCD_WriteReg(0x21,Xpos);
	LCD_WriteReg(0x20,Ypos);
#else//����	
	LCD_WriteReg(0x20,Xpos);
	LCD_WriteReg(0x21,Ypos);
#endif
	Lcd_WriteIndex(0x22);		
} 
/*************************************************
��������LCD_Set_Region
���ܣ�����lcd��ʾ�����ڴ�����д�������Զ�����
��ڲ�����xy�����յ�
����ֵ����
*************************************************/
//������ʾ����
void Lcd_SetRegion(unsigned char xStar, unsigned char yStar,unsigned char xEnd,unsigned char yEnd)
{
//#if USE_HORIZONTAL//��������˺���
// 	LCD_WriteReg(0x36,xEnd);
// 	LCD_WriteReg(0x37,xStar);
// 	LCD_WriteReg(0x38,yEnd);
// 	LCD_WriteReg(0x39,yStar);
// 	LCD_WriteReg(0x20,xStar);
// 	LCD_WriteReg(0x21,yStar);
// #else//����	
	LCD_WriteReg(0x38,xEnd);
	LCD_WriteReg(0x39,xStar);
	LCD_WriteReg(0x36,yEnd);
	LCD_WriteReg(0x37,yStar);
	LCD_WriteReg(0x21,xStar);
	LCD_WriteReg(0x20,yStar);
// #endif
	Lcd_WriteIndex(0x22);	
}

	
/*************************************************
��������LCD_DrawPoint
���ܣ���һ����
��ڲ�����xy�������ɫ����
����ֵ����
*************************************************/
void Gui_DrawPoint(unsigned short x,unsigned short y,unsigned short Data)
{
	Lcd_SetXY(x,y);
	Lcd_WriteData_16Bit(Data);

}    

/*************************************************
��������Lcd_Clear
���ܣ�ȫ����������
��ڲ����������ɫCOLOR
����ֵ����
*************************************************/
void Lcd_Clear(unsigned short Color)               
{	
   unsigned int i,m;
   Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
   for(i=0;i<X_MAX_PIXEL;i++)
    for(m=0;m<Y_MAX_PIXEL;m++)
    {	
	  	Lcd_WriteData_16Bit(Color);
    }   
}


/****************************************************************************
* ��    �ƣ�void spi_tft_init(void)
* ��    �ܣ�Һ����ʼ������
* ��ڲ�������
* ���ڲ�������
* ˵    ����Һ����ʼ��_ILI9225_176X220
****************************************************************************/
void spi_tft_init(void)
{	
	spi_init();//ʹ��ģ��SPI

	Lcd_Reset(); //Reset before LCD Init.

	//LCD Init For 2.2inch LCD Panel with ILI9225.	
	LCD_WriteReg(0x10, 0x0000); // Set SAP,DSTB,STB
	LCD_WriteReg(0x11, 0x0000); // Set APON,PON,AON,VCI1EN,VC
	LCD_WriteReg(0x12, 0x0000); // Set BT,DC1,DC2,DC3
	LCD_WriteReg(0x13, 0x0000); // Set GVDD
	LCD_WriteReg(0x14, 0x0000); // Set VCOMH/VCOML voltage
	delay_ms(40); // Delay 20 ms
	
	// Please follow this power on sequence
	LCD_WriteReg(0x11, 0x0018); // Set APON,PON,AON,VCI1EN,VC
	LCD_WriteReg(0x12, 0x1121); // Set BT,DC1,DC2,DC3
	LCD_WriteReg(0x13, 0x0063); // Set GVDD
	LCD_WriteReg(0x14, 0x3961); // Set VCOMH/VCOML voltage
	LCD_WriteReg(0x10, 0x0800); // Set SAP,DSTB,STB
	delay_ms(10); // Delay 10 ms
	LCD_WriteReg(0x11, 0x1038); // Set APON,PON,AON,VCI1EN,VC
	delay_ms(30); // Delay 30 ms
	
	
	LCD_WriteReg(0x02, 0x0100); // set 1 line inversion

#if USE_HORIZONTAL//��������˺���
	//R01H:SM=0,GS=0,SS=0 (for details,See the datasheet of ILI9225)
	LCD_WriteReg(0x01, 0x001C); // set the display line number and display direction
	//R03H:BGR=1,ID0=1,ID1=1,AM=1 (for details,See the datasheet of ILI9225)
	LCD_WriteReg(0x03, 0x1038); // set GRAM write direction .
#else//����
	//R01H:SM=0,GS=0,SS=1 (for details,See the datasheet of ILI9225)
	LCD_WriteReg(0x01, 0x011C); // set the display line number and display direction 
	//R03H:BGR=1,ID0=1,ID1=1,AM=0 (for details,See the datasheet of ILI9225)
	LCD_WriteReg(0x03, 0x1030); // set GRAM write direction.
#endif

	LCD_WriteReg(0x07, 0x0000); // Display off
	LCD_WriteReg(0x08, 0x0808); // set the back porch and front porch
	LCD_WriteReg(0x0B, 0x1100); // set the clocks number per line
	LCD_WriteReg(0x0C, 0x0000); // CPU interface
	LCD_WriteReg(0x0F, 0x0501); // Set Osc
	LCD_WriteReg(0x15, 0x0020); // Set VCI recycling
	LCD_WriteReg(0x20, 0x0000); // RAM Address
	LCD_WriteReg(0x21, 0x0000); // RAM Address
	
	//------------------------ Set GRAM area --------------------------------//
	LCD_WriteReg(0x30, 0x0000); 
	LCD_WriteReg(0x31, 0x00DB); 
	LCD_WriteReg(0x32, 0x0000); 
	LCD_WriteReg(0x33, 0x0000); 
	LCD_WriteReg(0x34, 0x00DB); 
	LCD_WriteReg(0x35, 0x0000); 
	LCD_WriteReg(0x36, 0x00AF); 
	LCD_WriteReg(0x37, 0x0000); 
	LCD_WriteReg(0x38, 0x00DB); 
	LCD_WriteReg(0x39, 0x0000); 
	
	
	// ---------- Adjust the Gamma 2.2 Curve -------------------//
	LCD_WriteReg(0x50, 0x0603); 
	LCD_WriteReg(0x51, 0x080D); 
	LCD_WriteReg(0x52, 0x0D0C); 
	LCD_WriteReg(0x53, 0x0205); 
	LCD_WriteReg(0x54, 0x040A); 
	LCD_WriteReg(0x55, 0x0703); 
	LCD_WriteReg(0x56, 0x0300); 
	LCD_WriteReg(0x57, 0x0400); 
	LCD_WriteReg(0x58, 0x0B00); 
	LCD_WriteReg(0x59, 0x0017); 
	
	
	
	LCD_WriteReg(0x0F, 0x0701); // Vertical RAM Address Position
	LCD_WriteReg(0x07, 0x0012); // Vertical RAM Address Position
	delay_ms(50); // Delay 50 ms
	LCD_WriteReg(0x07, 0x1017); // Vertical RAM Address Position  
	
}

/*************************************************
��������Lcd_Clear
���ܣ�ȫ����������
��ڲ����������ɫCOLOR
����ֵ����
*************************************************/
void lcd_clear(unsigned short color)               
{	
	unsigned int i,m;
	Lcd_SetRegion(0, 0, X_MAX_PIXEL - 1, Y_MAX_PIXEL - 1);
	for(i = 0; i < X_MAX_PIXEL; i++)
		for(m = 0; m < Y_MAX_PIXEL; m++)
		{	
			Lcd_WriteData_16Bit(color);
		}   
}