/******************** (C) COPYRIGHT 2009 www.armjishu.com ************************
* File Name          : ARMJISHU_TouchScreen_ADS7843.c
* Author             : www.armjishu.com Team
* Version            : V3.5.0
* Date               : 03/20/2010
* Description        :  ADS7843 Touch Screen
                        ADS7843_CS   is PB1
                        ADS7843_INT  is PC1
*******************************************************************************/
#include "TouchScreen.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include "stm32f4xx_exti.h"
#include "stm324xg_eval_lcd.h"

// A/D 通道选择命令字和工作寄存器
#define	CHX 	0x90//0x90 	//通道Y+的选择控制字	//0x94
#define	CHY 	0xD0//0xd0	//通道X+的选择控制字	//0xD4

/*=====================================================================*/
static void LCD_BIG_POINT(u16 xScreen, u16 yScreen);
void SPI_Config(void);
unsigned char SPI_WriteByte(unsigned char data);
u16 distence(u16 data1,u16 data2);
static void ADS7843_Rd_Addata(u16 *X_Addata,u16 *Y_Addata);
u16 TPReadX(void);
u16 TPReadY(void);

void ADS7843_InterruptConfig(void);

/*=====================================================================*/

void ADS7843_Init(void)
{
    SPI_Config();
    //ADS7843_InterruptConfig();
}
/*=====================================================================*/
/*=====================================================================*/


void SPI_Config(void) 
{ 
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Enable SPI2 and GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_SPI2_PORT, ENABLE);

  /*!< SPI Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_SPI2_CLK, ENABLE);

  GPIO_PinAFConfig(SPI2_GPIO_PORT, SPI2_SCK_SOURCE, GPIO_AF_SPI2);
  GPIO_PinAFConfig(SPI2_GPIO_PORT, SPI2_MISO_SOURCE, GPIO_AF_SPI2);
  GPIO_PinAFConfig(SPI2_GPIO_PORT, SPI2_MOSI_SOURCE, GPIO_AF_SPI2);
      
  /*!< Configure  pins: SCK */
  GPIO_InitStructure.GPIO_Pin = SPI2_SCK;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;//GPIO_PuPd_NOPULL;
  GPIO_Init(SPI2_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure  pins: MISO */
  GPIO_InitStructure.GPIO_Pin = SPI2_MISO;
  GPIO_Init(SPI2_GPIO_PORT, &GPIO_InitStructure);

  /*!< Configure  pins: MOSI */
  GPIO_InitStructure.GPIO_Pin = SPI2_MOSI;
  GPIO_Init(SPI2_GPIO_PORT, &GPIO_InitStructure);

  //Configure PF10 pin: TP_CS pin 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;  	//推挽输出
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;     //GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOG,&GPIO_InitStructure); 
  ADS7843_CS_HIGH() ;     // TPCS片选默认为高
    
  /* Configure PG.07 as input For TP_IRQ*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;            //上拉输入
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOG,&GPIO_InitStructure);

  /* SPI configuration *///初始化SPI结构体
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //SPI1设置为两线全双工
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //设置SPI1为主模式
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //SPI发送接收8位帧结构
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; //串行时钟在不操作时，时钟为低电平
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; //第一个时钟沿开始采样数据
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //NSS信号由软件（使用SSI位）管理
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; //SPI波特率预分频值为8
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //数据传输从MSB位开始
  SPI_InitStructure.SPI_CRCPolynomial = 7; //CRC值计算的多项式
  SPI_Init(SPI2, &SPI_InitStructure);  //根据SPI_InitStruct中指定的参数初始化外设SPI2寄存器
  
  /* Enable SPI2  *///使能SPI2外设
  SPI_Cmd(SPI2, ENABLE);

  //关闭同一组SPI管脚的其他SPI设备
  RCC_AHB1PeriphClockCmd(RCC_NRF24L01_CSN, ENABLE);          //使能GPIO的时钟
  GPIO_InitStructure.GPIO_Pin = NRF24L01_CSN;      
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;           //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIO_NRF24L01_CSN, &GPIO_InitStructure);
}

/*=====================================================================*/
unsigned char SPI_WriteByte(unsigned char data) 
{ 
 unsigned char Data = 0; 

   //Wait until the transmit buffer is empty 
  while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE)==RESET); 
  // Send the byte  
  SPI_I2S_SendData(SPI2,data); 

   //Wait until a data is received 
  while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_RXNE)==RESET); 
  // Get the received data 
  Data = SPI_I2S_ReceiveData(SPI2); 

  // Return the shifted data 
  return Data; 
}  

/*=====================================================================*/
/*=====================================================================*/
static void LCD_BIG_POINT(u16 xScreen, u16 yScreen)
 {
   LCD_SetPoint(xScreen, yScreen, LCD_COLOR_MAGENTA);
   //LCD_SetPoint(xScreen-1, yScreen, LCD_COLOR_MAGENTA);
   LCD_SetPoint(xScreen+1, yScreen, LCD_COLOR_MAGENTA);
   //LCD_SetPoint(xScreen, yScreen-1, LCD_COLOR_MAGENTA);
   LCD_SetPoint(xScreen, yScreen+1, LCD_COLOR_MAGENTA);
   //LCD_SetPoint(xScreen-1, yScreen-1, LCD_COLOR_MAGENTA);
   //LCD_SetPoint(xScreen-1, yScreen+1, LCD_COLOR_MAGENTA);
   //LCD_SetPoint(xScreen+1, yScreen-1, LCD_COLOR_MAGENTA);
   LCD_SetPoint(xScreen+1, yScreen+1, LCD_COLOR_MAGENTA);
 }
/*=====================================================================*/
/*
#define ShortDelayTime 1
#define LongDelayTime (ShortDelayTime*2)
SPI1_Delay_Short()
{
    __IO uint32_t nCount;
    
    for( nCount= 0; nCount < ShortDelayTime; nCount++ );
}
*/
/*=====================================================================*/
/*=====================================================================*/
u16 _AD2Y(u16 adx) //240
{
  u16 sx=0;
  int r = adx - 360;
  r *= 240;
  sx=r / (4096 - 800);
  if (sx<=0 || sx>240)
    return 0;
  return sx;
}

/*=====================================================================*/
u16 _AD2X(u16 ady) //320
{
  u16 sy=0;
  int r = ady - 360;
  r *= 320;
  sy=r/(4096 - 680);
  if (sy<=0 || sy>320)
    return 0;
  return sy;
}
/*=====================================================================*/
u16 TPReadX(void)
{ 
   u16 x=0;
   ADS7843_CS_LOW();
   SPI_WriteByte(CHX);
   x=SPI_WriteByte(0x00);
   x<<=8;
   x+=SPI_WriteByte(0x00);
   ADS7843_CS_HIGH(); 
   x = x>>3;
   return (x);
}

/*=====================================================================*/
u16 TPReadY(void)
{
  u16 y=0;
  ADS7843_CS_LOW();

  SPI_WriteByte(CHY);
  y=SPI_WriteByte(0x00);
  y<<=8;
  y+=SPI_WriteByte(0x00);
  ADS7843_CS_HIGH();
  y = y>>3; 
  return (y);
}

/*=====================================================================*/
#define  times  4
static void ADS7843_Rd_Addata(u16 *X_Addata,u16 *Y_Addata)
{

	u16		i,j,k,x_addata[times],y_addata[times];
    
	for(i=0;i<times;i++)					//采样4次.
	{
		x_addata[i] = TPReadX();
		y_addata[i] = TPReadY();
	}

    //交换法排序的程序最清晰简单，每次用当前的元素同其后的元素比较并交换
	for(i=0;i<times-1;i++)
	{
    	for(j=i+1;j<times;j++)
    	{
           if(x_addata[j] > x_addata[i])
            {
                k = x_addata[j];
                x_addata[j] = x_addata[i];
                x_addata[i] = k;
            }
         }
    }

    //交换法排序的程序最清晰简单，每次用当前的元素同其后的元素比较并交换
	for(i=0;i<times-1;i++)
	{
    	for(j=i+1;j<times;j++)
    	{
           if(y_addata[j] > y_addata[i])
            {
                k = y_addata[j];
                y_addata[j] = y_addata[i];
                y_addata[i] = k;
            }
         }
    }
	
	
	*X_Addata=(x_addata[1] + x_addata[2]) >> 1;
	*Y_Addata=(y_addata[1] + y_addata[2]) >> 1;

}


/*=====================================================================*/
/*=====================================================================*/
u16 distence(u16 data1,u16 data2)
{
    if((data2 > data1 + 2) || (data1 > data2 + 2))
    {
        return 0;
    }

    return 1;    
}

/*=====================================================================*/
void ARMJISHU_TouchScreen_ADS7843(void)
{
  u16 xdata, ydata;
  u32 xScreen, yScreen;
  static u16 sDataX,sDataY;
  static u16 Counter = 0;
    
  ADS7843_Rd_Addata(&xdata, &ydata);
  //printf("\n\r%d,%d", xdata, ydata);
  xScreen = _AD2X(xdata);
  yScreen = _AD2Y(ydata);

   //xScreen = 320 - ((ydata*320)>>12);
   //yScreen = (xdata*240)>>12;

  if((xScreen>1)&&(yScreen>1)&&(xScreen<320-1)&&(yScreen<240-1))
  {
      if((Counter % 1024) == 8)
      {
       printf("\n\r%d,%d", xScreen, yScreen);
       //printf("|%d,%d", xScreen, yScreen);
	  }
      if((GPIO_ADS7843_INT_VALID) && distence(sDataX,xScreen) && distence(sDataY,yScreen))
        {
             //LCD_BIG_POINT(xScreen, yScreen);
			 LCD_BIG_POINT(yScreen, xScreen);
        }
      sDataX = xScreen;
      sDataY = yScreen;
	  Counter++;
  }
    
}
/*=====================================================================*/
/*******************************************************************************
* Function Name  : InterruptConfig
* Description    : Configures the used IRQ Channels and sets their priority.NVIC_Configuration
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ADS7843_InterruptConfig(void)
{ 
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable SYSCFG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Connect EXTI Line0 to PA0 pin */
  SYSCFG_EXTILineConfig(GPIO_ADS7843_EXTI_PORT_SOURCE, GPIO_ADS7843_EXTI_PIN_SOURCE);
    
  /* Configure EXTI Line0 */
  EXTI_InitStructure.EXTI_Line = GPIO_ADS7843_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable the EXTI7 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = GPIO_ADS7843_EXTI_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


/*=====================================================================*/

