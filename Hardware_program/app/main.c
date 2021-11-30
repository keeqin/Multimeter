/****************************************Copyright (c)************************************************
**                                      [艾克姆科技]
**                                        IIKMSIK 
**                            官方店铺：https://acmemcu.taobao.com
**                            官方论坛：http://www.e930bbs.com
**                                   
**--------------File Info-----------------------------------------------------------------------------
** File name         : main.c
** Last modified Date: 2020-5-9        
** Last Version      :		   
** Descriptions      : 使用的SDK版本-SDK_16.0
**						
**----------------------------------------------------------------------------------------------------
** Created by        : [艾克姆]
** Created date      : 2020-4-21
** Version           : 1.0
** Descriptions      : 在“实验15-1：动态广播-定时更新广播中的厂商自定义数据”的基础上修改
**                   ：创建了一个超时时间为1秒的周期性APP定时器定时采样电位器抽头电压（模拟电池电压）
**                   ：SAADC事件处理函数中读取采样的电压值并计算成电池电量，同时读取片内温度传感器的温度值，
**                   ：当电池电量和温度两者有一个变化时，更新广播内容
**---------------------------------------------------------------------------------------------------*/
//引用的C库头文件
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//APP定时器需要引用的头文件
#include "app_timer.h"

#include "bsp_btn_ble.h"
//广播需要引用的头文件
#include "ble_advdata.h"
#include "ble_advertising.h"
//电源管理需要引用的头文件
#include "nrf_pwr_mgmt.h"
//SoftDevice handler configuration需要引用的头文件
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//排序写入模块需要引用的头文件
#include "nrf_ble_qwr.h"
//GATT需要引用的头文件
#include "nrf_ble_gatt.h"
//连接参数协商需要引用的头文件
#include "ble_conn_params.h"
//串口透传需要引用的头文件
#include "my_ble_uarts.h"
#include "my_ble_buttons.h"
#include "my_ble_voltage.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif
#include "app_uart.h"
#include "nrf_drv_saadc.h"


#define DEVICE_NAME                     "Multimeter"                      // 设备名称字符串 
#define UARTS_SERVICE_UUID_TYPE         BLE_UUID_TYPE_VENDOR_BEGIN         // 串口透传服务UUID类型：厂商自定义UUID
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)   // 最小连接间隔 (0.1 秒) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)   // 最大连接间隔 (0.2 秒) 
#define SLAVE_LATENCY                   0                                  // 从机延迟 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)    // 监督超时(4 秒) 
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)              // 定义首次调用sd_ble_gap_conn_param_update()函数更新连接参数延迟时间（5秒）
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)             // 定义每次调用sd_ble_gap_conn_param_update()函数更新连接参数的间隔时间（30秒）
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                  // 定义放弃连接参数协商前尝试连接参数协商的最大次数（3次）

#define APP_ADV_INTERVAL                320                                // 广播间隔 (200ms)，单位0.625 ms 
#define APP_ADV_DURATION                0                                  // 广播持续时间，单位：10ms。设置为0表示不超时 

#define APP_BLE_OBSERVER_PRIO           3               //应用程序BLE事件监视者优先级，应用程序不能修改该数值
#define APP_BLE_CONN_CFG_TAG            1               //SoftDevice BLE配置标志

#define UART_TX_BUF_SIZE 256                            //串口发送缓存大小（字节数）
#define UART_RX_BUF_SIZE 256                            //串口接收缓存大小（字节数）

//用于stack dump的错误代码，可以用于栈回退时确定堆栈位置
#define DEAD_BEEF                       0xDEADBEEF  

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600              //参考电压，单位mV，本例中SAADC的参考电压设置为0.6V，即600mV
#define ADC_PRE_SCALING_COMPENSATION    6                //SAADC的增益设置为1/6，所以结果要乘以6
#define ADC_RES_10BIT                   1024             //10位ADC转换的最大数值


BLE_VOLTAGE_DEF(m_voltage);
BLE_BUTTONS_DEF(m_buttons);
BLE_UARTS_DEF(m_uarts, NRF_SDH_BLE_TOTAL_LINK_COUNT);    //定义名称为m_uarts的串口透传服务实例
NRF_BLE_GATT_DEF(m_gatt);                                //定义名称为m_gatt的GATT模块实例
NRF_BLE_QWR_DEF(m_qwr);                                  //定义一个名称为m_qwr的排队写入实例
BLE_ADVERTISING_DEF(m_advertising);                      //定义名称为m_advertising的广播模块实例

//定时器超时时间：1000ms
#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(1000)              // 电池电量测试间隔：1秒
//定义APP定时器m_adv_updata_timer_id，用于更新广播内容
APP_TIMER_DEF(m_battery_timer_id);
APP_TIMER_DEF(m_diff_timer_id);

//该宏用于将SAADC采样值转换为电压，单位mV
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

//定义服务数据变量，因为使用了电池电量和温度服务数据，所以这里定义的是一个数组
ble_advdata_service_data_t service_data[2];
//SAADC采样缓存
static nrf_saadc_value_t adc_buf[2];
static nrf_saadc_value_t diff_adc_buf[2];
//定义保存电池电量的变量，电池电量初始值设置为100%
static uint8_t battery_level = 100;
//定义保存上一次电池电量的变量，以便和当次获取的电池电量对比有没有变化
static uint8_t battery_level_last;
//定义保存温度的变量
static uint32_t temperature_data = 100; 
//定义保存上一次温度的变量，以便和当次获取的温度对比有没有变化
static uint32_t temperature_data_last;

//该变量用于保存连接句柄，初始值设置为无连接
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; 
//发送的最大数据长度
static uint16_t   m_ble_uarts_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            
static bool uart_enabled = false;
//定义串口透传服务UUID列表
static ble_uuid_t m_adv_uuids[]          =                                          
{
    {BLE_UUID_UARTS_SERVICE, UARTS_SERVICE_UUID_TYPE}
};

//设备名称数组  中文名称：艾克姆串口透传
//const char device_name[21] = {0xE8,0x89,0xBE,0xE5,0x85,0x8B,0xE5,0xA7,0x86,0xE4,0xB8,0xB2,0xE5,0x8F,0xA3,0xE9,0x80,0x8F,0xE4,0xBC,0xA0};

//GAP参数初始化，该函数配置需要的GAP参数，包括设备名称，外观特征、首选连接参数
static void gap_params_init(void)
{
    ret_code_t              err_code;
	  //定义连接参数结构体变量
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
    //设置GAP的安全模式
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    //设置GAP设备名称，使用英文设备名称
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                              (const uint8_t *)DEVICE_NAME,
                                              strlen(DEVICE_NAME));
	
	  //设置GAP设备名称，这里使用了中文设备名称
//    err_code = sd_ble_gap_device_name_set(&sec_mode,
//                                          (const uint8_t *)device_name,
//                                          sizeof(device_name));
																					
    //检查函数返回的错误代码
		APP_ERROR_CHECK(err_code);
																				
    //设置首选连接参数，设置前先清零gap_conn_params
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
    //调用协议栈API sd_ble_gap_ppcp_set配置GAP参数
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
																					
}
//GATT事件处理函数，该函数中处理MTU交换事件
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    //如果是MTU交换事件
	  if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        //设置串口透传服务的有效数据长度（MTU-opcode-handle）
			  m_ble_uarts_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_uarts_max_data_len, m_ble_uarts_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}
//初始化GATT程序模块
static void gatt_init(void)
{
    //初始化GATT程序模块
	  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
	  //设置ATT MTU的大小,这里设置的值为247
	  err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}

//排队写入事件处理函数，用于处理排队写入模块的错误
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}
//串口事件回调函数，串口初始化时注册，该函数中判断事件类型并进行处理
//当接收的数据长度达到设定的最大值或者接收到换行符后，则认为一包数据接收完成，之后将接收的数据发送给主机
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_UARTS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;
    //判断事件类型
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY://串口接收事件
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;
            //接收串口数据，当接收的数据长度达到m_ble_uarts_max_data_len或者接收到换行符后认为一包数据接收完成
            if ((data_array[index - 1] == '\n') ||
                (data_array[index - 1] == '\r') ||
                (index >= m_ble_uarts_max_data_len))
            {
                if (index > 1)
                {
                    NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                    NRF_LOG_HEXDUMP_DEBUG(data_array, index);
                    //串口接收的数据使用notify发送给BLE主机
                    do
                    {
                        uint16_t length = (uint16_t)index;
                        err_code = ble_uarts_data_send(&m_uarts, data_array, &length, m_conn_handle);
                        if ((err_code != NRF_ERROR_INVALID_STATE) &&
                            (err_code != NRF_ERROR_RESOURCES) &&
                            (err_code != NRF_ERROR_NOT_FOUND))
                        {
                            APP_ERROR_CHECK(err_code);
                        }
                    } while (err_code == NRF_ERROR_RESOURCES);
                }

                index = 0;
            }
            break;
        //通讯错误事件，进入错误处理
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;
        //FIFO错误事件，进入错误处理
        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
//串口配置
void uart_config(void)
{
	uint32_t err_code;
	
	//定义串口通讯参数配置结构体并初始化
  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,//定义uart接收引脚
    TX_PIN_NUMBER,//定义uart发送引脚
    RTS_PIN_NUMBER,//定义uart RTS引脚，流控关闭后虽然定义了RTS和CTS引脚，但是驱动程序会忽略，不会配置这两个引脚，两个引脚仍可作为IO使用
    CTS_PIN_NUMBER,//定义uart CTS引脚
    APP_UART_FLOW_CONTROL_DISABLED,//关闭uart硬件流控
    false,//禁止奇偶检验
    NRF_UART_BAUDRATE_115200//uart波特率设置为115200bps
  };
  //初始化串口，注册串口事件回调函数
  APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_event_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

  APP_ERROR_CHECK(err_code);
	
}
static void uart_reconfig(void)
{
	if(uart_enabled == false)//初始化串口
	{
		uart_config();
		uart_enabled = true;
	}
	else
	{
		app_uart_close();//反初始化串口
		uart_enabled = false;
	}
}
//串口透传事件回调函数，串口透出服务初始化时注册
static void uarts_data_handler(ble_uarts_evt_t * p_evt)
{
	  //通知使能后才初始化串口
	  if (p_evt->type == BLE_NUS_EVT_COMM_STARTED)
		{
			uart_reconfig();
		}
		//通知关闭后，关闭串口
		else if(p_evt->type == BLE_NUS_EVT_COMM_STOPPED)
		{
		  uart_reconfig();
		}
	  //判断事件类型:接收到新数据事件
    if ((p_evt->type == BLE_UARTS_EVT_RX_DATA) && (uart_enabled == true))
    {
        uint32_t err_code;
        //串口打印出接收的数据
        for (uint32_t i = 0; i < p_evt->params.rx_data.length; i++)
        {
            do
            {
                err_code = app_uart_put(p_evt->params.rx_data.p_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
        if (p_evt->params.rx_data.p_data[p_evt->params.rx_data.length - 1] == '\r')
        {
            while (app_uart_put('\n') == NRF_ERROR_BUSY);
        }
    }
		//判断事件类型:发送就绪事件，该事件在后面的试验会用到，当前我们在该事件中翻转指示灯D4的状态，指示该事件的产生
    if (p_evt->type == BLE_UARTS_EVT_TX_RDY)
    {
			nrf_gpio_pin_toggle(LED_4);
		}
}
//服务初始化，包含初始化排队写入模块和初始化应用程序使用的服务
static void services_init(void)
{
    ret_code_t         err_code;
	  //定义串口透传初始化结构体
	  ble_uarts_init_t     uarts_init;
	  //定义排队写入初始化结构体变量
    nrf_ble_qwr_init_t qwr_init = {0};

    //排队写入事件处理函数
    qwr_init.error_handler = nrf_qwr_error_handler;
    //初始化排队写入模块
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
		//检查函数返回值
    APP_ERROR_CHECK(err_code);
    
		
		/*------------------以下代码初始化串口透传服务-------------*/
		//清零串口透传服务初始化结构体
//		memset(&uarts_init, 0, sizeof(uarts_init));
//		//设置串口透传事件回调函数
//    uarts_init.data_handler = uarts_data_handler;
//    //初始化串口透传服务
//    err_code = ble_uarts_init(&m_uarts, &uarts_init);
//    APP_ERROR_CHECK(err_code);
		/*------------------初始化串口透传服务-END-----------------*/
//		err_code =  ble_buttons_init(&m_buttons);
//		APP_ERROR_CHECK(err_code);
		ble_voltage_init(&m_voltage);
		
		
		
		
}

//连接参数协商模块事件处理函数
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;
    //判断事件类型，根据事件类型执行动作
	  //连接参数协商失败，断开当前连接
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
		//连接参数协商成功
		if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
       //功能代码;
    }
}

//连接参数协商模块错误处理事件，参数nrf_error包含了错误代码，通过nrf_error可以分析错误信息
static void conn_params_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}


//连接参数协商模块初始化
static void conn_params_init(void)
{
    ret_code_t             err_code;
	  //定义连接参数协商模块初始化结构体
    ble_conn_params_init_t cp_init;
    //配置之前先清零
    memset(&cp_init, 0, sizeof(cp_init));
    //设置为NULL，从主机获取连接参数
    cp_init.p_conn_params                  = NULL;
	  //连接或启动通知到首次发起连接参数更新请求之间的时间设置为5秒
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	  //每次调用sd_ble_gap_conn_param_update()函数发起连接参数更新请求的之间的间隔时间设置为：30秒
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	  //放弃连接参数协商前尝试连接参数协商的最大次数设置为：3次
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	  //连接参数更新从连接事件开始计时
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	  //连接参数更新失败不断开连接
    cp_init.disconnect_on_fail             = false;
	  //注册连接参数更新事件句柄
    cp_init.evt_handler                    = on_conn_params_evt;
	  //注册连接参数更新错误事件句柄
    cp_init.error_handler                  = conn_params_error_handler;
    //调用库函数（以连接参数更新初始化结构体为输入参数）初始化连接参数协商模块
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

//广播事件处理函数
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;
    //判断广播事件类型
    switch (ble_adv_evt)
    {
        //快速广播启动事件：快速广播启动后会产生该事件
			  case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
			      //设置广播指示灯为正在广播（D1指示灯闪烁）
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        //广播IDLE事件：广播超时后会产生该事件
        case BLE_ADV_EVT_IDLE:
					  //设置广播指示灯为广播停止（D1指示灯熄灭）
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}
//读取片内温度传感器
uint32_t temperature_data_get(void)
{
    int32_t temp;
    uint32_t err_code;
    
    err_code = sd_temp_get(&temp);
    APP_ERROR_CHECK(err_code);
    
    temp = (temp / 4) * 100;
    
    int8_t exponent = -2;
    return ((exponent & 0xFF) << 24) | (temp & 0x00FFFFFF);
}
//广播初始化
static void advertising_init(void)
{
    ret_code_t             err_code;

	  //定义一个变量保存温度(片内温度传感器读取的温度)
    uint32_t temperature_data = temperature_data_get();
	
	  //定义广播初始化配置结构体变量
    ble_advertising_init_t init;
    //配置之前先清零
    memset(&init, 0, sizeof(init));
    //设备名称类型：全称
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	  //是否包含外观：包含
    init.advdata.include_appearance      = false;
	  //Flag:一般可发现模式，不支持BR/EDR
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		
		//电池电量服务UUID 0x180F
		service_data[0].service_uuid = BLE_UUID_BATTERY_SERVICE;
    service_data[0].data.size    = sizeof(battery_level);
    service_data[0].data.p_data  = &battery_level;
    //温度服务UUID 0x1809
    service_data[1].service_uuid = BLE_UUID_HEALTH_THERMOMETER_SERVICE;
    service_data[1].data.size    = sizeof(temperature_data);
    service_data[1].data.p_data  = (uint8_t *) &temperature_data;


    //广播数据中加入服务数据
	  init.advdata.p_service_data_array = service_data;
		//服务数据数量设置为2
	  init.advdata.service_data_count = 2;
		
	  
	  //UUID放到扫描响应里面
	  init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;
	
    //设置广播模式为快速广播
    init.config.ble_adv_fast_enabled  = true;
	  //设置广播间隔和广播持续时间
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    //广播事件回调函数
    init.evt_handler = on_adv_evt;
    //初始化广播
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);
    //设置广播配置标记。APP_BLE_CONN_CFG_TAG是用于跟踪广播配置的标记，这是为未来预留的一个参数，在将来的SoftDevice版本中，
		//可以使用sd_ble_gap_adv_set_configure()配置新的广播配置
		//当前SoftDevice版本（S132 V7.0.1版本）支持的最大广播集数量为1，因此APP_BLE_CONN_CFG_TAG只能写1。
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

//BLE事件处理函数
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
    //判断BLE事件类型，根据事件类型执行相应操作
    switch (p_ble_evt->header.evt_id)
    {
        //断开连接事件
			  case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
				    
				    //打印提示信息
				    NRF_LOG_INFO("Disconnected.");
				    uart_reconfig();
				    //连接断开后，重新启动APP定时器
            err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
            APP_ERROR_CHECK(err_code);
            break;
				
        //连接事件
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
				    //设置指示灯状态为连接状态，即指示灯D1常亮
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
				    //保存连接句柄
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
				    //将连接句柄分配给排队写入实例，分配后排队写入实例和该连接关联，这样，当有多个连接的时候，通过关联不同的排队写入实例，很方便单独处理各个连接
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
				    //连接建立后，停止更新广播内容
				    //err_code = app_timer_stop(m_battery_timer_id);
				    APP_ERROR_CHECK(err_code);
						
            break;
				
        //PHY更新事件
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
						//响应PHY更新规程
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
				//安全参数请求事件
				case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            //不支持配对
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
				 
				//系统属性访问正在等待中
				case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            //系统属性没有存储，更新系统属性
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;
        //GATT客户端超时事件
        case BLE_GATTC_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Client Timeout.");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        //GATT服务器超时事件
        case BLE_GATTS_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Server Timeout.");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}

//初始化BLE协议栈
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //请求使能SoftDevice，该函数中会根据sdk_config.h文件中低频时钟的设置来配置低频时钟
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //定义保存应用程序RAM起始地址的变量
    uint32_t ram_start = 0;
	  //使用sdk_config.h文件的默认参数配置协议栈，获取应用程序RAM起始地址，保存到变量ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //使能BLE协议栈
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    //注册BLE事件回调函数
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}
//初始化电源管理模块
static void power_management_init(void)
{
    ret_code_t err_code;
	  //初始化电源管理
    err_code = nrf_pwr_mgmt_init();
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
}

//初始化指示灯
static void leds_init(void)
{
    ret_code_t err_code;
	
    //初始化BSP指示灯
    err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}
//更新广播内容
static void adv_updata(uint8_t bat_data,uint32_t temp_data)
{
	  ret_code_t err_code;
	  ble_advdata_t           adv_data; //广播数据
    ble_advdata_t           sr_data;  //扫描响应数据
	
	
	  //电池电量服务UUID 0x180F
		service_data[0].service_uuid = BLE_UUID_BATTERY_SERVICE;
    service_data[0].data.size    = sizeof(bat_data);
    service_data[0].data.p_data  = &bat_data;
    //温度服务UUID 0x1809
    service_data[1].service_uuid = BLE_UUID_HEALTH_THERMOMETER_SERVICE;
    service_data[1].data.size    = sizeof(temp_data);
    service_data[1].data.p_data  = (uint8_t *) &temp_data;

	  //先清零，再配置
	  memset(&adv_data, 0, sizeof(adv_data));
	  memset(&sr_data, 0, sizeof(sr_data));
	  adv_data.name_type               = BLE_ADVDATA_FULL_NAME;
	  //是否包含外观：包含
    adv_data.include_appearance      = false;
	  //Flag:一般可发现模式，不支持BR/EDR
    adv_data.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		
		//广播数据中加入服务数据
	  adv_data.p_service_data_array = service_data;
		//服务数据数量设置为2
	  adv_data.service_data_count = 2;
			
	  //UUID放到扫描响应里面
	  sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids  = m_adv_uuids;
	  //更新广播内容
	  err_code = ble_advertising_advdata_update(&m_advertising, &adv_data, &sr_data);
		APP_ERROR_CHECK(err_code);
}
//SAADC事件处理函数
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
	  if (p_event->type == NRF_DRV_SAADC_EVT_DONE)//SAADC缓存满事件
    {
			  nrf_saadc_value_t adc_result;
        uint16_t          batt_lvl_in_milli_volts;
        uint32_t          err_code;
         
        //设置好缓存，为下一次采样准备
        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);
			
			  //读取采样值
        adc_result = p_event->data.done.p_buffer[0];
				char buf[20];
				
				sprintf(buf,"{\"v\":%5d,\"u\":\"v\"}",adc_result);
				uint16_t len = strlen(buf);
			
			ble_voltage_data_send(&m_voltage,buf,&len,m_conn_handle);
			
        //采样数值转换为电压，单位mV
        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result);
			  //电压转换为电量
        battery_level = battery_level_in_percent(batt_lvl_in_milli_volts);
			
			  //定义一个变量保存温度(片内温度传感器读取的温度)
        temperature_data = temperature_data_get();
			  //只有当温度或者电池电量改变时才更新广播内容
			  if((battery_level != battery_level_last) || (temperature_data != temperature_data_last))
				{
					//更新广播内容
	        adv_updata(battery_level,temperature_data);
					//保存当前数据，以便下一次对比
					battery_level_last = battery_level;
					temperature_data_last = temperature_data;		
				}
    }
}
//SAADC初始化，使用双缓存
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
//        NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
	NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_DIFFERENTIAL(NRF_SAADC_INPUT_AIN3, NRF_SAADC_INPUT_AIN2);
	  //初始化SAADC，注册事件回调函数
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);
    //配置缓存1，将缓存1地址赋值给SAADC驱动程序中的控制块m_cb的一级缓存指针
    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);
    //配置缓存2，将缓存1地址赋值给SAADC驱动程序中的控制块m_cb的二级缓存指针
    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
}

//void diff_saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
//{
//	  if (p_event->type == NRF_DRV_SAADC_EVT_DONE)//SAADC缓存满事件
//    {
//				nrf_saadc_value_t adc_result;
//        uint16_t          batt_lvl_in_milli_volts;
//        uint32_t          err_code;
//         
//        //设置好缓存，为下一次采样准备
//        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
//        APP_ERROR_CHECK(err_code);
//			
//			  //读取采样值
//        adc_result = p_event->data.done.p_buffer[0];
//				char buf[20];
//				uint16_t len = 5;
//				sprintf(buf,"%5d",adc_result);
//				ble_voltage_data_send(&m_voltage,buf,&len,m_conn_handle);
//		}
//}

//static void diff_adc_configure(void)
//{
//		ret_code_t err_code = nrf_drv_saadc_init(NULL, diff_saadc_event_handler);
//		APP_ERROR_CHECK(err_code);
//		nrf_saadc_channel_config_t config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_DIFFERENTIAL(NRF_SAADC_INPUT_VDD, NRF_SAADC_INPUT_AIN2);
//		err_code = nrf_drv_saadc_channel_init(1, &config);
//		APP_ERROR_CHECK(err_code);
//	//配置缓存1，将缓存1地址赋值给SAADC驱动程序中的控制块m_cb的一级缓存指针
//    err_code = nrf_drv_saadc_buffer_convert(&diff_adc_buf[0], 1);
//    APP_ERROR_CHECK(err_code);
//    //配置缓存2，将缓存1地址赋值给SAADC驱动程序中的控制块m_cb的二级缓存指针
//    err_code = nrf_drv_saadc_buffer_convert(&diff_adc_buf[1], 1);
//    APP_ERROR_CHECK(err_code);
//}

//APP定时器超时事件回调函数：该函数中启动一次SAADC采样
static void adv_updata_timeout_handler(void * p_context)
{
	  ret_code_t err_code;
	  //防止编译器警告，同时清晰地的表明p_context未使用，而不是应用程序忽略了。如果应用程序需要使用p_context，则不需要这行代码
	  UNUSED_PARAMETER(p_context);
	  //启动一次ADC采样
	  err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}
//初始化APP定时器模块
static void timers_init(void)
{
    //初始化APP定时器模块
    ret_code_t err_code = app_timer_init();
	  //检查返回值
    APP_ERROR_CHECK(err_code);

    //创建APP定时器：电池测量APP定时器，周期定时器，启动后周期运行，用于电池测量
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                adv_updata_timeout_handler);
	
	  APP_ERROR_CHECK(err_code); 

}
//启动已经创建的的APP定时器
static void application_timers_start(void)
{
    ret_code_t err_code;

    //启动电池测量APP定时器
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);

}
static void log_init(void)
{
    //初始化log程序模块
	  ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //设置log输出终端（根据sdk_config.h中的配置设置输出终端为UART或者RTT）
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//空闲状态处理函数。如果没有挂起的日志操作，则睡眠直到下一个事件发生后唤醒系统
static void idle_state_handle(void)
{
    //处理挂起的log
	  if (NRF_LOG_PROCESS() == false)
    {
        //运行电源管理，该函数需要放到主循环里面执行
			  nrf_pwr_mgmt_run();
    }
}
//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
static void advertising_start(void)
{
   //使用广播初始化中设置的广播模式启动广播
	 ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
	 //检查函数返回的错误代码
   APP_ERROR_CHECK(err_code);
}
void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
	uint8_t buf[21];
	uint16_t len = 20
	;
	if(button_action == APP_BUTTON_PUSH)                                                                                 
	{
		sprintf((char*)buf,"11111111111111111111");
	}else{
		sprintf((char*)buf,"00000000000000000000");
	}
	
	ble_buttons_data_send(&m_buttons,buf,&len,m_conn_handle);
	nrf_gpio_pin_toggle(LED_2);
}
void button_config(void)
{
	uint32_t err_code;
	static app_button_cfg_t buttons[] = {
	{BUTTON_1, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL , button_event_handler},
	{BUTTON_2, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL , button_event_handler},
	{BUTTON_3, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL , button_event_handler},
	{BUTTON_4, APP_BUTTON_ACTIVE_LOW, BUTTON_PULL , button_event_handler},
	};
	app_button_init(buttons,ARRAY_SIZE(buttons),10);
	err_code = app_button_enable();
	APP_ERROR_CHECK(err_code);
}

//主函数
int main(void)
{
	//初始化log程序模块
	log_init();
	//初始化串口
	//uart_config();
	button_config();
	//初始化APP定时器
	timers_init();
	//出使唤按键和指示灯
	leds_init();
	//初始化电源管理
	power_management_init();
	//初始化协议栈
	ble_stack_init();
	//初始化SAADC
	adc_configure();
	//配置GAP参数
	gap_params_init();
	//初始化GATT
	gatt_init();
	//初始化服务
	services_init();
	//初始化广播
	advertising_init();	
	//连接参数协商初始化
  conn_params_init();
	
	
  NRF_LOG_INFO("BLE Template example started."); 
  //启动APP定时器	
	application_timers_start();	
	//启动广播
	advertising_start();
  //主循环
	while(true)
	{
		//处理挂起的LOG和运行电源管理
		idle_state_handle();
	}
}

