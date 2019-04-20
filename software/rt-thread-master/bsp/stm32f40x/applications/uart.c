/*
 * uart.c
 *
 *  Created on: 2019年2月30日
 *      Author: zengwangfa
 *      Notes:  串口设备读写数据
 */
#define LOG_TAG    "uart"

#include "gyroscope.h"
#include <elog.h>
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_ano.h"
#include "uart.h"
#include "RC_Data.h"

/*---------------------- Constant / Macro Definitions -----------------------*/
#define CONTROL_UART_NAME     "uart1"
#define GYRO_UART_NAME        "uart2"

#define DEBUG_UART_NAME       "uart3"   //可更改为uart3 WIFI 、 uart4 蓝牙
#define FOCUS_UART_NAME       "uart5"   //可更改为uart3 WIFI 、 uart4 蓝牙

#define Query_JY901_data 0     /* "1"为调试查询  "0"为正常读取 */

#if Query_JY901_data
char recv_buffer[128]; 				//串口2接收数据缓冲变量,
unsigned char recv_data_p=0x00;  //  串口2接收数据指针
#endif

/*----------------------- Variable Declarations -----------------------------*/
/* ALL_init 事件控制块. */
extern struct rt_event init_event;

rt_device_t control_uart_device;	
rt_device_t debug_uart_device;	
rt_device_t gyro_uart_device;	
rt_device_t focus_uart_device;	

struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT; /* 配置参数 */

struct rt_semaphore control_rx_sem;/* 用于接收消息的信号量 */
struct rt_semaphore gyro_rx_sem;/* 用于接收消息的信号量 */
struct rt_semaphore debug_rx_sem;/* 用于接收消息的信号量 */
struct rt_semaphore focus_rx_sem;/* 用于接收消息的信号量 */

uint8 debug_startup_flag = 0; //debug串口 初始化完成标志位
/*------------------------------------------ Control Uart ------------------------------------------------*/
/* 接收数据回调函数 */
static rt_err_t control_uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&control_rx_sem);

    return RT_EOK;
}

static void control_thread_entry(void *parameter)
{
    unsigned char ch;

		while (1)
		{
				/* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
				while (rt_device_read(control_uart_device, 0, &ch, 1) != 1)
				{
				  /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
						rt_sem_take(&control_rx_sem, RT_WAITING_FOREVER);
				}
				Remote_Control_Data_Analysis(ch);
		}
}

/*------------------------------------------ Gyro Uart ------------------------------------------------*/
/* 接收数据回调函数 */
static rt_err_t gyroscope_uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&gyro_rx_sem);

    return RT_EOK;
}

static void gyroscope_thread_entry(void *parameter)
{
    unsigned char ch;

		while (1)
		{
				/* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
				while (rt_device_read(gyro_uart_device, 0, &ch, 1) != 1)
				{
				  /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
						rt_sem_take(&gyro_rx_sem, RT_WAITING_FOREVER);
				}
#if Query_JY901_data //在线调试 查询模式
				
				recv_buffer[recv_data_p] = ch;
				recv_data_p++;
				if(recv_data_p>127)recv_data_p = 0;
		
#else 
				CopeSerial2Data(ch); //正常传输模式 筛选数据包
#endif
		}
}

/*------------------------------------------ Debug Uart ------------------------------------------------*/
/* 接收数据回调函数 */
static rt_err_t debug_uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&debug_rx_sem);

    return RT_EOK;
}


static void debug_thread_entry(void *parameter)
{
    unsigned char ch;

		while (1)
		{
				/* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
				while (rt_device_read(debug_uart_device, 0, &ch, 1) != 1)
				{
						/* 阻塞等待接收信号量，等到信号量后再次读取数据 */
						rt_sem_take(&debug_rx_sem, RT_WAITING_FOREVER);
				}
				ANO_DT_Data_Receive_Prepare(ch);

		}
}

/*------------------------------------------ Focus Uart ------------------------------------------------*/
/* 接收数据回调函数 */
static rt_err_t focus_uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&focus_rx_sem);

    return RT_EOK;
}


static void focus_thread_entry(void *parameter)
{
    unsigned char ch;

		while (1)
		{
				/* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
				while (rt_device_read(focus_uart_device, 0, &ch, 1) != 1)
				{
						/* 阻塞等待接收信号量，等到信号量后再次读取数据 */
						rt_sem_take(&focus_rx_sem, RT_WAITING_FOREVER);
				}


		}
}


/*----------------------- Function Implement --------------------------------*/
int device_uart_init(void)
{
		uint8 uart_init_flag = 1;
		/* 串口 线程句柄 */
		rt_thread_t control_tid;
		rt_thread_t gyroscope_tid;
		rt_thread_t debug_tid;
		rt_thread_t focus_tid;
	
	  /* 查找系统中的串口设备 */

		control_uart_device = rt_device_find(CONTROL_UART_NAME); 
		gyro_uart_device = rt_device_find(GYRO_UART_NAME);       
		debug_uart_device = rt_device_find(DEBUG_UART_NAME);
	  focus_uart_device = rt_device_find(FOCUS_UART_NAME);

		log_v("control serial: %s", control_uart_device);	
		log_v("gyroscope serial: %s", gyro_uart_device);
		log_v("debug serial: %s", debug_uart_device);
		log_v("console serial: %s", RT_CONSOLE_DEVICE_NAME);	
		log_v("focus serial: %s", focus_uart_device);
	
    if (control_uart_device != RT_NULL){		
			
					/* 以读写以及中断接打开串口设备 */
				rt_device_open(control_uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);

				rt_sem_init(&control_rx_sem, "control_sem", 0, RT_IPC_FLAG_FIFO);
				/* 设置接收回调函数 */
				rt_device_set_rx_indicate(control_uart_device, control_uart_input);
		}	
		
    if (gyro_uart_device != RT_NULL){		
			
					/* 以读写以及中断接打开串口设备 */
				rt_device_open(gyro_uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);

				rt_sem_init(&gyro_rx_sem, "gyro_sem", 0, RT_IPC_FLAG_FIFO);
				/* 设置接收回调函数 */
				rt_device_set_rx_indicate(gyro_uart_device, gyroscope_uart_input);
		}
		
		if(debug_uart_device != RT_NULL){
				rt_device_open(debug_uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
				config.baud_rate = BAUD_RATE_115200;
				config.data_bits = DATA_BITS_8;
				config.stop_bits = STOP_BITS_1;
				config.parity = PARITY_NONE;
				
				log_v("Debug_Uart_Baud Rate:%d",config.baud_rate);
				/* 打开设备后才可修改串口配置参数 */
				rt_device_control(debug_uart_device, RT_DEVICE_CTRL_CONFIG, &config);
			
				rt_sem_init(&debug_rx_sem, "debug_sem", 0, RT_IPC_FLAG_FIFO);
				/* 设置接收回调函数 */
				rt_device_set_rx_indicate(debug_uart_device, debug_uart_input);
		}
		
		if (focus_uart_device != RT_NULL){		
			
					/* 以读写以及中断接打开串口设备 */
				rt_device_open(focus_uart_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);

				rt_sem_init(&focus_rx_sem, "focus_sem", 0, RT_IPC_FLAG_FIFO);
				/* 设置接收回调函数 */
				rt_device_set_rx_indicate(focus_uart_device, focus_uart_input);
		}
    /* 创建 控制 serial 线程 */
		control_tid = rt_thread_create("control_uart",
																		control_thread_entry,
																		RT_NULL, 
																		512, 
																		12,
																		1);
    /* 创建 九轴 serial 线程 */
		gyroscope_tid = rt_thread_create("gyro_uart",
																			gyroscope_thread_entry,
																			RT_NULL, 
																			512, 
																			12,
																			1);
		
		/* 创建 调试 serial 线程 */
		debug_tid = rt_thread_create("debug_uart",
																	debug_thread_entry,
																	RT_NULL, 
																	512, 
																	13,
																	1);
																			
		/* 创建 变焦 serial 线程 */
		focus_tid = rt_thread_create("focus_uart",
																	focus_thread_entry,
																	RT_NULL, 
																	512, 
																	14,
																	1);
		/* 创建成功则启动线程 */
    if (control_tid != RT_NULL){
				rt_thread_startup(control_tid);
				uart_init_flag <<= 1;
				//rt_event_send(&init_event, GYRO_EVENT); //发送事件  表示初始化完成
    }
    /* 创建成功则启动线程 */
    if (gyroscope_tid != RT_NULL){
				rt_thread_startup(gyroscope_tid);
				uart_init_flag <<= 1;
    }
		
		/* 创建成功则启动线程 */
    if (debug_tid != RT_NULL){
				rt_thread_startup(debug_tid);
				debug_startup_flag = 1;
				uart_init_flag <<= 1;
    }
		
				/* 创建成功则启动线程 */
    if (focus_tid != RT_NULL){
				rt_thread_startup(focus_tid);
				uart_init_flag <<= 1;
    }
		
		if(0x10 == uart_init_flag){
				log_i("Uart_Init()");
		}
		else {
			  log_e("Uart_Init have some errors.");
		}
		return 0;
}
INIT_APP_EXPORT(device_uart_init);





