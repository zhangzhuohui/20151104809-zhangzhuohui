/***************STM32F103C8T6**********************
 * �ļ���  ��main.c
 * ����    ��������ӡ��UART test��,ͨ�����ڵ������������ӷ������ݣ����ӽ��յ����ݺ������ش������ԡ�         
 * ʵ��ƽ̨��STM32F103C8T6  ��rom��64kb������ռ�ÿռ䣩  ram��20kb����������ռ���ڴ�ռ䣩��
 * ��汾  ��ST3.0.0  																										  
 ��Яʽ���ڼ�ȩ���ϵͳ
********************LIGEN*************************/

#include "stm32f10x.h"    //�ٷ�����ļ�       
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
//����ı���
#define FLASH_SAVE_ADDR  0X0800fe00		//����FLASH �����ַ(����Ϊż��������ֵҪ���ڱ�������ռ��FLASH�Ĵ�С+0X08000000)
//#define __debug 0 //���ڵ���
u8 IRQURA3=0;//����3 ����������ϱ�־
u8 IRQURA2=0;//����2 ����������ϱ�־
u8 USART2_RX_BUF[USART2_REC_LEN]={0};//����2���ݷ���ȥ
u8 USART3_RX_BUF[USART3_REC_LEN]={0};//����2���ݷ���ȥ 

//----------------------------------------------
u8 temperature=0; //�¶� 	    
u8 humidity=0;     //ʪ��
u16 SmokeScope=0; //����Ũ�� ��λppm  
u16 SmokeWarningTemp=500; //������ֵ
u8 DisFlag=0; //��ʾ�л���־ 
 
float  CH2OTemp=0;         //��ȩ����
//----------------------------------------------
u8 timecount1=0;//��ʱ����ʱ
 
//��ʼ��ʱ��־
u8 timeFlag1=1;

 /*
unsigned char = u8
unsigned short int = u16
unsigned long int =u32
*/
 
 //��Ƭ��д�����ݵĺ���
void write_flash()
{

	u16 data[1]={0};//Ҫд�������
	//д��   
	//data[0]=icNum;
	//data[1]=yyFlag;
	//д��   ���ֽ�ǰ ���ֽ� ��
	//д�� 1 ��
	data[0]=SmokeWarningTemp;
	STMFLASH_Write(FLASH_SAVE_ADDR,data,1); //STM32_IAP(IN Application Programing(API Application programing InterfaceӦ�ó����̽ӿ�))
	/*
	
	*/
	//printf("write flash=%d\r\n",icNum );
} 
 
/*
�ӵ�Ƭ����flash���洢��������Ҫ����Ĳ���
*/
void read_flash()
{
	u16 data[1]={0};
	STMFLASH_Read(FLASH_SAVE_ADDR,data,1);
	if(data[0]==0xffff) 
		return;
	//ȡ����
 SmokeWarningTemp=data[0];
	//yyFlag =data[1];
}
 
 
 //Һ����ʾ����
 
 void disVoltage()
{
	u16 ch2o=0; 
	if(DisFlag==0)//����1 û�а���
	{	
		//------------��ȩ��ʾ--------------//	
			USART_Cmd(USART2, DISABLE);                    //�رմ���2
			ch2o=CH2OTemp;
			LCD_8x16Ch(48,0,ch2o/1000);
			OLED_P8x16Str(48+8,0,".");  //m  
			LCD_8x16Ch(48+16,0,ch2o%1000/100%10);
			LCD_8x16Ch(48+24,0,ch2o%1000%100/10%10);
			LCD_8x16Ch(48+32,0,ch2o%10);	
			USART_Cmd(USART2, ENABLE);                    //�򿪴���2
		//------------�¶�------------//
			LCD_8x16Ch(48+16,2,temperature/10%10);	
			LCD_8x16Ch(48+24,2,temperature%10); 	 
		//------------ʪ��------------//
			LCD_8x16Ch(48+16,4,humidity/10%10);	
			LCD_8x16Ch(48+24,4,humidity%10); 
		//------------����Ũ��------------//
			LCD_8x16Ch(48+8,6,SmokeScope/1000%10);	
			LCD_8x16Ch(48+16,6,SmokeScope%1000/100%10); 
			LCD_8x16Ch(48+24,6,SmokeScope%100/10%10); 
			LCD_8x16Ch(48+32,6,SmokeScope%10); 	
	}
	else //DisFlag��=0 ����1����
	{
		//------------����Ũ��------------//
			LCD_8x16Ch(48+8,4,SmokeWarningTemp/1000%10);	
			LCD_8x16Ch(48+16,4,SmokeWarningTemp%1000/100%10); 
			LCD_8x16Ch(48+24,4,SmokeWarningTemp%100/10%10); 
			LCD_8x16Ch(48+32,4,SmokeWarningTemp%10%10); 		
	
	}
}
void DisInit()
{
    	OLED_Fill(0x00); //��ʼ����
	    OLED_P8x16Str(0,0,"C");  //: 0 0��ʾ��Ļ�ĺ�������
	    OLED_P8x16Str(8,0,"H");  //m ������
	    OLED_P8x16Str(16,0,"2");  //:
	    OLED_P8x16Str(24,0,"O");  //m ������
	    OLED_P8x16Str(32,0,":");  //:
	    OLED_P8x16Str(88,0,"mg/m3");  //m ������
 
			
	    OLED_P8x16Str(0,2,"T");  //:
	    OLED_P8x16Str(8,2,"e");  //m ������
	    OLED_P8x16Str(16,2,"m");  //:
	    OLED_P8x16Str(32,2,":");  //:	

	    OLED_P8x16Str(88,2,"C");  //m ������

	    OLED_P8x16Str(0,4,"H");  //:
	    OLED_P8x16Str(8,4,"u");  //m ������
	    OLED_P8x16Str(16,4,"m");  //:
	    OLED_P8x16Str(32,4,":");  //:	 


	    OLED_P8x16Str(88,4,"%");  //m ������			
			
	    OLED_P8x16Str(0,6,"S");  //:
	    OLED_P8x16Str(8,6,"m");  //m ������
	    OLED_P8x16Str(16,6,"o");  //:
	    OLED_P8x16Str(24,6,"k");  //:	
	    OLED_P8x16Str(32,6,"e");  //:	
	    OLED_P8x16Str(40,6,":");  //:
	    OLED_P8x16Str(88+8,6,"ppm");  //m ������		

}
void DisSet()
{
	    OLED_Fill(0x00); //��ʼ����
	    OLED_P8x16Str(0,0,"Set WarningTemp");  //
		
			
	    OLED_P8x16Str(0,4,"S");  //:
	    OLED_P8x16Str(8,4,"m");  //m ������
	    OLED_P8x16Str(16,4,"o");  //:
	    OLED_P8x16Str(24,4,"k");  //:	
	    OLED_P8x16Str(32,4,"e");  //:	
	    OLED_P8x16Str(40,4,":");  //:
	    OLED_P8x16Str(88+8,4,"ppm");  //m ������		

}
void Beeps()//����
{
  if(SmokeScope>SmokeWarningTemp)//����ʵ��Ũ��ֵ>����Ũ���趨����ֵ
	{
	 Beep=1; //��������
	}
else
	Beep=0; 

}
int  main(void)  
{  
	u8 i=0;
	u8 t=0;			    
  u8 temp[50]={0};	
	 SystemInit();	//����ϵͳʱ��Ϊ 72M 
	 delay_init();	    	 //��ʱ������ʼ��	  
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	 //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ� 
	 LED_Init();
	 //uart1_init(9600); //wifi
	 ESP8266_Init();      //8266��ʼ��
	 usart2_init(9600);//�ɼ���ȩ����
	 usart3_init(115200);//���õ������ 
	 TIM3_Int_Init(4999,7199);//10Khz�ļ���Ƶ�ʣ�������5000Ϊ500ms  
	 LED_Init();		  	//��ʼ����LED���ӵ�Ӳ���ӿ�
	 OLED_IOInit();          //OLED��ʼ��
	 OLED_Init();          //OLED��ʼ��
	 DisInit();
	 read_flash();//����Ũ�Ⱦ�����ʼ����ֵ���´��ϵ绹�ᱣ��
  ESP8266_AT_Test (); //ATָ�����
	ESP8266_Net_Mode_Choose (STA_AP); //����ģʽ  ������ģʽ
  ESP8266_Enable_MultipleId ( ENABLE );  //����Ϊ������ģʽ
	ESP8266_StartOrShutServer(ENABLE,"8080","100"); //����������ģʽ���˿�8080 
  
	Adc_Init();//��ʼ��AD
	while(DHT11_Init())	//DHT11��ʼ��	 �򿪶˿�
	{ 
	delay_ms(200);
	}		
	
		while (1)  
		{
			Beeps();//����������
 
			if(!KEY1) //������� 
			{
			  while(!KEY1);
			 	printf ("press key1 \r\n");
				if(DisFlag == 0)
				DisFlag = 1;//������ԭ���ǵ�һ�ΰ��½�������ģʽ���ٰ����˳�����ģʽ��������Ҫ��DisFlag�÷�
				else 
				DisFlag = 0;
				if(DisFlag == 0) //������ģʽ����֮����Ҫ��ʼ������LED����ʾ
				DisInit();
			else
			   DisSet();	
         disVoltage();			
			}
   if(DisFlag!=0) //����ģʽ
	 {
	  if(!KEY2) //KEY2���µĻ����Ǽ�100
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
	else // ������ʾģʽ
	 {
	   getCH2OTemp(); 
	     if(t%10==0)//ÿ100ms��ȡһ��
				{									  
					DHT11_Read_Data(&temperature,&humidity);		//��ȡ��ʪ��ֵ					    
				}				   
	 	    delay_ms(10);
		    t++; 
			 if(timecount1>4) //2���ϴ�һ�� ͨ��WIFIģ�鷢�ͼ�ȩ���¶ȣ�ʪ�ȣ�������ֵ
			 {
				 disVoltage();
				 SmokeScope=Get_Adc_Average(0,10);
				 timecount1=0; 
				 sprintf ((char*)temp,"��ȩ=%0.3f mg/m3\n�¶�=%d ��\nʪ��=%d %% \n����=%d ppm\r\n",
				 CH2OTemp/1000,temperature,humidity,SmokeScope); //ͨ��wifi�ϴ���APP
				 if(DisFlag==0)
				 ESP8266_SendString(DISABLE,temp,strlen(temp),Multiple_ID_0);
			 }				
	 }		
		}
}








