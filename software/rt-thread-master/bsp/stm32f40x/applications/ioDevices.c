
/*
 * key.c
 *
 *  Created on: 2019��2��30��
 *      Author: zengwangfa
 *      Notes:  IO �豸���ƣ�KEY��BUZZER��
 */


#define LOG_TAG  "ioDevices"//Tag

#include <drivers/pin.h>
#include <elog.h>
#include "ioDevices.h"
#include "oled.h"

/*---------------------- Constant / Macro Definitions -----------------------*/
#define KEY_PIN  						88 	 //PDG3   ����IO

#define WIFI_CONNECT_PIN    80 	 //PD11   WIFI����IO���
#define WIFI_RELOAD_PIN     68 	 //PE15   WIFI��λIO

#define BOMA3_PIN 					13	 //PF3   ���뿪��IO
#define BOMA2_PIN	  				14	 //PF4
#define BOMA1_PIN 					15	 //PF5  

#define Buzzer_PIN 					59   //PE8   ������IO

#define boma1_read 					rt_pin_read(BOMA1_PIN)
#define boma2_read 					rt_pin_read(BOMA2_PIN)
#define boma3_read 					rt_pin_read(BOMA3_PIN)

#define wifi_read 					rt_pin_read(WIFI_CONNECT_PIN)
/*----------------------- Variable Declarations -----------------------------*/
/* ALL_init �¼����ƿ�. */
extern struct rt_event init_event;

Buzzer_Type Beep;  //������������
uint8 boma_value = 0;	//�ݴ沦��״̬ �жϲ���״̬�Ƿ�ı�

/*----------------------- Function Implement --------------------------------*/

void ioDevices_thread_entry(void* parameter)// --- KEY   BOMA ---
{
		if(System_NORMAL_STATUS == boma_value_get())
				//Buzzer_Set(&Beep,3,1);	
    while (1)
    {
				Buzzer_Process(&Beep); //��������������
				if(boma_value != boma_value_get()){ //�����뿪�� ��������������һ��
						Buzzer_Set(&Beep,1,1);	
						boma_value = boma_value_get();	
						log_i("\nCurrent Change: BOMA_Value = %d", boma_value);
				}
				rt_thread_mdelay(10);
    }
}


/* get 2λ����ֵ */
uint8 boma_value_get(void)
{
    uint8 val; //reserve(�洢)
    
		val = boma1_read *2 + boma2_read *1 + 1; //�õ�����״̬
    return val;
}

/**
	* @brief  wifi_connect_get(wifi����)
  * @param  None
  * @retval 1 ������   0������
  * @notice 
  */
uint8 wifi_connect_get(void)
{
		return !wifi_read; 
}

/* �������²��������� */
void key_down(void *args)  
{
				oled.pagenum ++;				
}








/*buzzerΪ������������  countΪ��Ĵ���  length���ʱ�䳤��  */
void Buzzer_Set(Buzzer_Type* buzzer,uint8_t count,uint8_t length)
{
		buzzer->count  = count *2;	//���벻��Ϊ ����״̬������ *2
		buzzer->time   = length*5; //����ʱ��������
		buzzer->number = length*5; //��ʱ�䳤��
}  

/* �������������񡾿�ָʾϵͳ����״̬�� */
void Buzzer_Process(Buzzer_Type * buzzer)
{
		if(buzzer->count >= 1)//�����Ĵ������ڵ���1
		{  
				if(buzzer->number >= 1){
						if(1 == buzzer->count%2)   {rt_pin_write(Buzzer_PIN, PIN_HIGH);}//����һ����
						else                    	{rt_pin_write(Buzzer_PIN, PIN_LOW) ;} 
						buzzer->number--;	//ʱ���� ��1
				}
				else{
						buzzer->number = buzzer->time;//��һ����� �����¸�ֵ
						buzzer->count--;  //������1
				}
		}
		else
		{
				rt_pin_write(Buzzer_PIN, PIN_LOW); //��ɺ����͡����졿
				buzzer->time  = 0;
				buzzer->count = 0;
		}
}



int ioDevices_thread_init(void)
{
   rt_thread_t ioDecices_tid;

    ioDecices_tid = rt_thread_create("ioDev",ioDevices_thread_entry, RT_NULL,512,25, 10);

    if (ioDecices_tid != RT_NULL){			
				rt_pin_mode(KEY_PIN , PIN_MODE_INPUT_PULLUP);    //���ܰ��������뿪��  ��������
				rt_pin_mode(BOMA1_PIN, PIN_MODE_INPUT_PULLUP);  //���뿪��  ��������
				rt_pin_mode(BOMA2_PIN, PIN_MODE_INPUT_PULLUP);  
				rt_pin_mode(BOMA3_PIN, PIN_MODE_INPUT_PULLUP);  
	
				rt_pin_mode(WIFI_RELOAD_PIN, PIN_MODE_OUTPUT);  //WIFI ����IO���
				rt_pin_write(WIFI_RELOAD_PIN ,PIN_HIGH);
			
				rt_pin_mode (Buzzer_PIN, PIN_MODE_OUTPUT);  //���ģʽ
				rt_pin_write(Buzzer_PIN, PIN_LOW);
			
				rt_pin_attach_irq(KEY_PIN, PIN_IRQ_MODE_FALLING, key_down, RT_NULL);/* ���жϣ�������ģʽ���ص�������Ϊkey_down */
				rt_pin_irq_enable(KEY_PIN, PIN_IRQ_ENABLE);/* ʹ���ж� */
			

				boma_value = boma_value_get();	//��ʼ���õ���ǰ����״̬ --> VehicleStatus
				log_i("IoDev_Init()");
				rt_thread_startup(ioDecices_tid);
		}
		return 0;
}
INIT_APP_EXPORT(ioDevices_thread_init);

/*��WIFI������MSH���� */
int wifi_reload(void)
{
		rt_pin_write(WIFI_RELOAD_PIN ,PIN_LOW);
		rt_thread_mdelay(4000);
		rt_pin_write(WIFI_RELOAD_PIN ,PIN_HIGH);
		rt_thread_mdelay(1000);
		log_d("WIFI Reload!\r\n");
		return 0;

}
MSH_CMD_EXPORT(wifi_reload,wifi reload);



