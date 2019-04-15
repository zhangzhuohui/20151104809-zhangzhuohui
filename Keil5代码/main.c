/***************STM32F103C8T6**********************
 * 文件名  ：main.c
 * 描述    ：开机打印“UART test”,通过串口调试软件，向板子发送数据，板子接收到数据后，立即回传给电脑。         
 * 实验平台：STM32F103C8T6  【rom：64kb（程序占用空间）  ram：20kb（程序运行占用内存空间）】
 * 库版本  ：ST3.0.0  																										  
 便携式室内甲醛检测系统
********************LIGEN*************************/

#include "stm32f10x.h"    //官方库的文件       
#include "sys.h"   
#include "main.h"
#include "usart1.h"
#include "usart2.h" 
#include "usart3.h"   
#include "delay.h"  
#include "led.h"
#include "rc522.h" 
#include "rc522_add.h"
#include "stmflash.h"
#include "adc.h"
#include "timer.h"
#include "dhT11.h"
#include "bsp_esp8266.h"
//定义的变量
#define FLASH_SAVE_ADDR  0X0800fe00		//设置FLASH 保存地址(必须为偶数，且其值要大于本代码所占用FLASH的大小+0X08000000)
//#define __debug 0 //用于调试
u8 IRQURA3=0;//串口3 接受数据完毕标志
u8 IRQURA2=0;//串口2 接受数据完毕标志
u8 USART2_RX_BUF[USART2_REC_LEN]={0};//串口2数据反冲去
u8 USART3_RX_BUF[USART3_REC_LEN]={0};//串口2数据反冲去 

//----------------------------------------------
u8 temperature=0; //温度 	    
u8 humidity=0;     //湿度
u16 SmokeScope=0; //烟雾浓度 单位ppm  
u16 SmokeWarningTemp=500; //烟雾报警值
u8 DisFlag=0; //显示切换标志 
 
float  CH2OTemp=0;         //甲醛含量
//----------------------------------------------
u8 timecount1=0;//定时器延时
 
//开始计时标志
u8 timeFlag1=1;

 /*
unsigned char = u8
unsigned short int = u16
unsigned long int =u32
*/
 
 //向单片机写入数据的函数
void write_flash()
{

	u16 data[1]={0};//要写入的数据
	//写入   
	//data[0]=icNum;
	//data[1]=yyFlag;
	//写入   低字节前 高字节 后
	//写入 1 个
	data[0]=SmokeWarningTemp;
	STMFLASH_Write(FLASH_SAVE_ADDR,data,1); //STM32_IAP(IN Application Programing(API Application programing Interface应用程序编程接口))
	/*
	
	*/
	//printf("write flash=%d\r\n",icNum );
} 
 
/*
从单片机的flash（存储器）读出要保存的参数
*/
void read_flash()
{
	u16 data[1]={0};
	STMFLASH_Read(FLASH_SAVE_ADDR,data,1);
	if(data[0]==0xffff) 
		return;
	//取数据
 SmokeWarningTemp=data[0];
	//yyFlag =data[1];
}
 
 
 //液晶显示函数
 
 void disVoltage()
{
	u16 ch2o=0; 
	if(DisFlag==0)//按键1 没有按下
	{	
		//------------甲醛显示--------------//	
			USART_Cmd(USART2, DISABLE);                    //关闭串口2
			ch2o=CH2OTemp;
			LCD_8x16Ch(48,0,ch2o/1000);
			OLED_P8x16Str(48+8,0,".");  //m  
			LCD_8x16Ch(48+16,0,ch2o%1000/100%10);
			LCD_8x16Ch(48+24,0,ch2o%1000%100/10%10);
			LCD_8x16Ch(48+32,0,ch2o%10);	
			USART_Cmd(USART2, ENABLE);                    //打开串口2
		//------------温度------------//
			LCD_8x16Ch(48+16,2,temperature/10%10);	
			LCD_8x16Ch(48+24,2,temperature%10); 	 
		//------------湿度------------//
			LCD_8x16Ch(48+16,4,humidity/10%10);	
			LCD_8x16Ch(48+24,4,humidity%10); 
		//------------烟雾浓度------------//
			LCD_8x16Ch(48+8,6,SmokeScope/1000%10);	
			LCD_8x16Ch(48+16,6,SmokeScope%1000/100%10); 
			LCD_8x16Ch(48+24,6,SmokeScope%100/10%10); 
			LCD_8x16Ch(48+32,6,SmokeScope%10); 	
	}
	else //DisFlag！=0 按键1按下
	{
		//------------烟雾浓度------------//
			LCD_8x16Ch(48+8,4,SmokeWarningTemp/1000%10);	
			LCD_8x16Ch(48+16,4,SmokeWarningTemp%1000/100%10); 
			LCD_8x16Ch(48+24,4,SmokeWarningTemp%100/10%10); 
			LCD_8x16Ch(48+32,4,SmokeWarningTemp%10%10); 		
	
	}
}
void DisInit()
{
    	OLED_Fill(0x00); //初始清屏
	    OLED_P8x16Str(0,0,"C");  //: 0 0显示屏幕的横纵坐标
	    OLED_P8x16Str(8,0,"H");  //m 立方米
	    OLED_P8x16Str(16,0,"2");  //:
	    OLED_P8x16Str(24,0,"O");  //m 立方米
	    OLED_P8x16Str(32,0,":");  //:
	    OLED_P8x16Str(88,0,"mg/m3");  //m 立方米
 
			
	    OLED_P8x16Str(0,2,"T");  //:
	    OLED_P8x16Str(8,2,"e");  //m 立方米
	    OLED_P8x16Str(16,2,"m");  //:
	    OLED_P8x16Str(32,2,":");  //:	

	    OLED_P8x16Str(88,2,"C");  //m 立方米

	    OLED_P8x16Str(0,4,"H");  //:
	    OLED_P8x16Str(8,4,"u");  //m 立方米
	    OLED_P8x16Str(16,4,"m");  //:
	    OLED_P8x16Str(32,4,":");  //:	 


	    OLED_P8x16Str(88,4,"%");  //m 立方米			
			
	    OLED_P8x16Str(0,6,"S");  //:
	    OLED_P8x16Str(8,6,"m");  //m 立方米
	    OLED_P8x16Str(16,6,"o");  //:
	    OLED_P8x16Str(24,6,"k");  //:	
	    OLED_P8x16Str(32,6,"e");  //:	
	    OLED_P8x16Str(40,6,":");  //:
	    OLED_P8x16Str(88+8,6,"ppm");  //m 立方米		

}
void DisSet()
{
	    OLED_Fill(0x00); //初始清屏
	    OLED_P8x16Str(0,0,"Set WarningTemp");  //
		
			
	    OLED_P8x16Str(0,4,"S");  //:
	    OLED_P8x16Str(8,4,"m");  //m 立方米
	    OLED_P8x16Str(16,4,"o");  //:
	    OLED_P8x16Str(24,4,"k");  //:	
	    OLED_P8x16Str(32,4,"e");  //:	
	    OLED_P8x16Str(40,4,":");  //:
	    OLED_P8x16Str(88+8,4,"ppm");  //m 立方米		

}
void Beeps()//报警
{
  if(SmokeScope>SmokeWarningTemp)//烟雾实际浓度值>烟雾浓度设定警报值
	{
	 Beep=1; //蜂鸣器响
	}
else
	Beep=0; 

}
int  main(void)  
{  
	u8 i=0;
	u8 t=0;			    
  u8 temp[50]={0};	
	 SystemInit();	//配置系统时钟为 72M 
	 delay_init();	    	 //延时函数初始化	  
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级 
	 LED_Init();
	 //uart1_init(9600); //wifi
	 ESP8266_Init();      //8266初始化
	 usart2_init(9600);//采集甲醛含量
	 usart3_init(115200);//备用调试输出 
	 TIM3_Int_Init(4999,7199);//10Khz的计数频率，计数到5000为500ms  
	 LED_Init();		  	//初始化与LED连接的硬件接口
	 OLED_IOInit();          //OLED初始化
	 OLED_Init();          //OLED初始化
	 DisInit();
	 read_flash();//烟雾浓度警报初始化的值，下次上电还会保存
  ESP8266_AT_Test (); //AT指令测试
	ESP8266_Net_Mode_Choose (STA_AP); //设置模式  服务器模式
  ESP8266_Enable_MultipleId ( ENABLE );  //设置为多链接模式
	ESP8266_StartOrShutServer(ENABLE,"8080","100"); //开启服务器模式，端口8080 
  
	Adc_Init();//初始化AD
	while(DHT11_Init())	//DHT11初始化	 打开端口
	{ 
	delay_ms(200);
	}		
	
		while (1)  
		{
			Beeps();//蜂鸣器报警
 
			if(!KEY1) //按键检测 
			{
			  while(!KEY1);
			 	printf ("press key1 \r\n");
				if(DisFlag == 0)
				DisFlag = 1;//按键的原理是第一次按下进入设置模式，再按下退出设置模式，所以需要将DisFlag置反
				else 
				DisFlag = 0;
				if(DisFlag == 0) //从设置模式出来之后需要初始化以下LED的显示
				DisInit();
			else
			   DisSet();	
         disVoltage();			
			}
   if(DisFlag!=0) //设置模式
	 {
	  if(!KEY2) //KEY2按下的话就是加100
		 {
			  while(!KEY2);
		  if(SmokeWarningTemp<2000)
				SmokeWarningTemp+=100;
							 disVoltage();
			 write_flash();
		 }
	  if(!KEY3)
		 {
			  while(!KEY3);
		  if(SmokeWarningTemp>100)
				SmokeWarningTemp-=100;
							 disVoltage();
			write_flash();
		 }	 
	 }
	else // 正常显示模式
	 {
	   getCH2OTemp(); 
	     if(t%10==0)//每100ms读取一次
				{									  
					DHT11_Read_Data(&temperature,&humidity);		//读取温湿度值					    
				}				   
	 	    delay_ms(10);
		    t++; 
			 if(timecount1>4) //2秒上传一次 通过WIFI模块发送甲醛，温度，湿度，烟雾数值
			 {
				 disVoltage();
				 SmokeScope=Get_Adc_Average(0,10);
				 timecount1=0; 
				 sprintf ((char*)temp,"甲醛=%0.3f mg/m3\n温度=%d ℃\n湿度=%d %% \n烟雾=%d ppm\r\n",
				 CH2OTemp/1000,temperature,humidity,SmokeScope); //通过wifi上传到APP
				 if(DisFlag==0)
				 ESP8266_SendString(DISABLE,temp,strlen(temp),Multiple_ID_0);
			 }				
	 }		
		}
}








