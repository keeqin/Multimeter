/****************************************Copyright (c)************************************************
**                                      [����ķ�Ƽ�]
**                                        IIKMSIK 
**                            �ٷ����̣�https://acmemcu.taobao.com
**                            �ٷ���̳��http://www.e930bbs.com
**                                   
**--------------File Info-----------------------------------------------------------------------------
** File name         : main.c
** Last modified Date: 2020-5-9        
** Last Version      :		   
** Descriptions      : ʹ�õ�SDK�汾-SDK_16.0
**						
**----------------------------------------------------------------------------------------------------
** Created by        : [����ķ]
** Created date      : 2020-4-21
** Version           : 1.0
** Descriptions      : �ڡ�ʵ��15-1����̬�㲥-��ʱ���¹㲥�еĳ����Զ������ݡ��Ļ������޸�
**                   ��������һ����ʱʱ��Ϊ1���������APP��ʱ����ʱ������λ����ͷ��ѹ��ģ���ص�ѹ��
**                   ��SAADC�¼��������ж�ȡ�����ĵ�ѹֵ������ɵ�ص�����ͬʱ��ȡƬ���¶ȴ��������¶�ֵ��
**                   ������ص������¶�������һ���仯ʱ�����¹㲥����
**---------------------------------------------------------------------------------------------------*/
//���õ�C��ͷ�ļ�
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//Log��Ҫ���õ�ͷ�ļ�
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
//APP��ʱ����Ҫ���õ�ͷ�ļ�
#include "app_timer.h"

#include "bsp_btn_ble.h"
//�㲥��Ҫ���õ�ͷ�ļ�
#include "ble_advdata.h"
#include "ble_advertising.h"
//��Դ������Ҫ���õ�ͷ�ļ�
#include "nrf_pwr_mgmt.h"
//SoftDevice handler configuration��Ҫ���õ�ͷ�ļ�
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
//����д��ģ����Ҫ���õ�ͷ�ļ�
#include "nrf_ble_qwr.h"
//GATT��Ҫ���õ�ͷ�ļ�
#include "nrf_ble_gatt.h"
//���Ӳ���Э����Ҫ���õ�ͷ�ļ�
#include "ble_conn_params.h"
//����͸����Ҫ���õ�ͷ�ļ�
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


#define DEVICE_NAME                     "Multimeter"                      // �豸�����ַ��� 
#define UARTS_SERVICE_UUID_TYPE         BLE_UUID_TYPE_VENDOR_BEGIN         // ����͸������UUID���ͣ������Զ���UUID
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)   // ��С���Ӽ�� (0.1 ��) 
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)   // ������Ӽ�� (0.2 ��) 
#define SLAVE_LATENCY                   0                                  // �ӻ��ӳ� 
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)    // �ල��ʱ(4 ��) 
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)              // �����״ε���sd_ble_gap_conn_param_update()�����������Ӳ����ӳ�ʱ�䣨5�룩
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)             // ����ÿ�ε���sd_ble_gap_conn_param_update()�����������Ӳ����ļ��ʱ�䣨30�룩
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                  // ����������Ӳ���Э��ǰ�������Ӳ���Э�̵���������3�Σ�

#define APP_ADV_INTERVAL                320                                // �㲥��� (200ms)����λ0.625 ms 
#define APP_ADV_DURATION                0                                  // �㲥����ʱ�䣬��λ��10ms������Ϊ0��ʾ����ʱ 

#define APP_BLE_OBSERVER_PRIO           3               //Ӧ�ó���BLE�¼����������ȼ���Ӧ�ó������޸ĸ���ֵ
#define APP_BLE_CONN_CFG_TAG            1               //SoftDevice BLE���ñ�־

#define UART_TX_BUF_SIZE 256                            //���ڷ��ͻ����С���ֽ�����
#define UART_RX_BUF_SIZE 256                            //���ڽ��ջ����С���ֽ�����

//����stack dump�Ĵ�����룬��������ջ����ʱȷ����ջλ��
#define DEAD_BEEF                       0xDEADBEEF  

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600              //�ο���ѹ����λmV��������SAADC�Ĳο���ѹ����Ϊ0.6V����600mV
#define ADC_PRE_SCALING_COMPENSATION    6                //SAADC����������Ϊ1/6�����Խ��Ҫ����6
#define ADC_RES_10BIT                   1024             //10λADCת���������ֵ


BLE_VOLTAGE_DEF(m_voltage);
BLE_BUTTONS_DEF(m_buttons);
BLE_UARTS_DEF(m_uarts, NRF_SDH_BLE_TOTAL_LINK_COUNT);    //��������Ϊm_uarts�Ĵ���͸������ʵ��
NRF_BLE_GATT_DEF(m_gatt);                                //��������Ϊm_gatt��GATTģ��ʵ��
NRF_BLE_QWR_DEF(m_qwr);                                  //����һ������Ϊm_qwr���Ŷ�д��ʵ��
BLE_ADVERTISING_DEF(m_advertising);                      //��������Ϊm_advertising�Ĺ㲥ģ��ʵ��

//��ʱ����ʱʱ�䣺1000ms
#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(1000)              // ��ص������Լ����1��
//����APP��ʱ��m_adv_updata_timer_id�����ڸ��¹㲥����
APP_TIMER_DEF(m_battery_timer_id);
APP_TIMER_DEF(m_diff_timer_id);

//�ú����ڽ�SAADC����ֵת��Ϊ��ѹ����λmV
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

//����������ݱ�������Ϊʹ���˵�ص������¶ȷ������ݣ��������ﶨ�����һ������
ble_advdata_service_data_t service_data[2];
//SAADC��������
static nrf_saadc_value_t adc_buf[2];
static nrf_saadc_value_t diff_adc_buf[2];
//���屣���ص����ı�������ص�����ʼֵ����Ϊ100%
static uint8_t battery_level = 100;
//���屣����һ�ε�ص����ı������Ա�͵��λ�ȡ�ĵ�ص����Ա���û�б仯
static uint8_t battery_level_last;
//���屣���¶ȵı���
static uint32_t temperature_data = 100; 
//���屣����һ���¶ȵı������Ա�͵��λ�ȡ���¶ȶԱ���û�б仯
static uint32_t temperature_data_last;

//�ñ������ڱ������Ӿ������ʼֵ����Ϊ������
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; 
//���͵�������ݳ���
static uint16_t   m_ble_uarts_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            
static bool uart_enabled = false;
//���崮��͸������UUID�б�
static ble_uuid_t m_adv_uuids[]          =                                          
{
    {BLE_UUID_UARTS_SERVICE, UARTS_SERVICE_UUID_TYPE}
};

//�豸��������  �������ƣ�����ķ����͸��
//const char device_name[21] = {0xE8,0x89,0xBE,0xE5,0x85,0x8B,0xE5,0xA7,0x86,0xE4,0xB8,0xB2,0xE5,0x8F,0xA3,0xE9,0x80,0x8F,0xE4,0xBC,0xA0};

//GAP������ʼ�����ú���������Ҫ��GAP�����������豸���ƣ������������ѡ���Ӳ���
static void gap_params_init(void)
{
    ret_code_t              err_code;
	  //�������Ӳ����ṹ�����
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;
    //����GAP�İ�ȫģʽ
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    //����GAP�豸���ƣ�ʹ��Ӣ���豸����
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                              (const uint8_t *)DEVICE_NAME,
                                              strlen(DEVICE_NAME));
	
	  //����GAP�豸���ƣ�����ʹ���������豸����
//    err_code = sd_ble_gap_device_name_set(&sec_mode,
//                                          (const uint8_t *)device_name,
//                                          sizeof(device_name));
																					
    //��麯�����صĴ������
		APP_ERROR_CHECK(err_code);
																				
    //������ѡ���Ӳ���������ǰ������gap_conn_params
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;//��С���Ӽ��
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;//��С���Ӽ��
    gap_conn_params.slave_latency     = SLAVE_LATENCY;    //�ӻ��ӳ�
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //�ල��ʱ
    //����Э��ջAPI sd_ble_gap_ppcp_set����GAP����
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
																					
}
//GATT�¼����������ú����д���MTU�����¼�
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    //�����MTU�����¼�
	  if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        //���ô���͸���������Ч���ݳ��ȣ�MTU-opcode-handle��
			  m_ble_uarts_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_uarts_max_data_len, m_ble_uarts_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}
//��ʼ��GATT����ģ��
static void gatt_init(void)
{
    //��ʼ��GATT����ģ��
	  ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	  //��麯�����صĴ������
    APP_ERROR_CHECK(err_code);
	  //����ATT MTU�Ĵ�С,�������õ�ֵΪ247
	  err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}

//�Ŷ�д���¼������������ڴ����Ŷ�д��ģ��Ĵ���
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    //���������
	  APP_ERROR_HANDLER(nrf_error);
}
//�����¼��ص����������ڳ�ʼ��ʱע�ᣬ�ú������ж��¼����Ͳ����д���
//�����յ����ݳ��ȴﵽ�趨�����ֵ���߽��յ����з�������Ϊһ�����ݽ�����ɣ�֮�󽫽��յ����ݷ��͸�����
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_UARTS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;
    //�ж��¼�����
    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY://���ڽ����¼�
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;
            //���մ������ݣ������յ����ݳ��ȴﵽm_ble_uarts_max_data_len���߽��յ����з�����Ϊһ�����ݽ������
            if ((data_array[index - 1] == '\n') ||
                (data_array[index - 1] == '\r') ||
                (index >= m_ble_uarts_max_data_len))
            {
                if (index > 1)
                {
                    NRF_LOG_DEBUG("Ready to send data over BLE NUS");
                    NRF_LOG_HEXDUMP_DEBUG(data_array, index);
                    //���ڽ��յ�����ʹ��notify���͸�BLE����
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
        //ͨѶ�����¼������������
        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;
        //FIFO�����¼������������
        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}
//��������
void uart_config(void)
{
	uint32_t err_code;
	
	//���崮��ͨѶ�������ýṹ�岢��ʼ��
  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,//����uart��������
    TX_PIN_NUMBER,//����uart��������
    RTS_PIN_NUMBER,//����uart RTS���ţ����عرպ���Ȼ������RTS��CTS���ţ����������������ԣ������������������ţ����������Կ���ΪIOʹ��
    CTS_PIN_NUMBER,//����uart CTS����
    APP_UART_FLOW_CONTROL_DISABLED,//�ر�uartӲ������
    false,//��ֹ��ż����
    NRF_UART_BAUDRATE_115200//uart����������Ϊ115200bps
  };
  //��ʼ�����ڣ�ע�ᴮ���¼��ص�����
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
	if(uart_enabled == false)//��ʼ������
	{
		uart_config();
		uart_enabled = true;
	}
	else
	{
		app_uart_close();//����ʼ������
		uart_enabled = false;
	}
}
//����͸���¼��ص�����������͸�������ʼ��ʱע��
static void uarts_data_handler(ble_uarts_evt_t * p_evt)
{
	  //֪ͨʹ�ܺ�ų�ʼ������
	  if (p_evt->type == BLE_NUS_EVT_COMM_STARTED)
		{
			uart_reconfig();
		}
		//֪ͨ�رպ󣬹رմ���
		else if(p_evt->type == BLE_NUS_EVT_COMM_STOPPED)
		{
		  uart_reconfig();
		}
	  //�ж��¼�����:���յ��������¼�
    if ((p_evt->type == BLE_UARTS_EVT_RX_DATA) && (uart_enabled == true))
    {
        uint32_t err_code;
        //���ڴ�ӡ�����յ�����
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
		//�ж��¼�����:���;����¼������¼��ں����������õ�����ǰ�����ڸ��¼��з�תָʾ��D4��״̬��ָʾ���¼��Ĳ���
    if (p_evt->type == BLE_UARTS_EVT_TX_RDY)
    {
			nrf_gpio_pin_toggle(LED_4);
		}
}
//�����ʼ����������ʼ���Ŷ�д��ģ��ͳ�ʼ��Ӧ�ó���ʹ�õķ���
static void services_init(void)
{
    ret_code_t         err_code;
	  //���崮��͸����ʼ���ṹ��
	  ble_uarts_init_t     uarts_init;
	  //�����Ŷ�д���ʼ���ṹ�����
    nrf_ble_qwr_init_t qwr_init = {0};

    //�Ŷ�д���¼�������
    qwr_init.error_handler = nrf_qwr_error_handler;
    //��ʼ���Ŷ�д��ģ��
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
		//��麯������ֵ
    APP_ERROR_CHECK(err_code);
    
		
		/*------------------���´����ʼ������͸������-------------*/
		//���㴮��͸�������ʼ���ṹ��
//		memset(&uarts_init, 0, sizeof(uarts_init));
//		//���ô���͸���¼��ص�����
//    uarts_init.data_handler = uarts_data_handler;
//    //��ʼ������͸������
//    err_code = ble_uarts_init(&m_uarts, &uarts_init);
//    APP_ERROR_CHECK(err_code);
		/*------------------��ʼ������͸������-END-----------------*/
//		err_code =  ble_buttons_init(&m_buttons);
//		APP_ERROR_CHECK(err_code);
		ble_voltage_init(&m_voltage);
		
		
		
		
}

//���Ӳ���Э��ģ���¼�������
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;
    //�ж��¼����ͣ������¼�����ִ�ж���
	  //���Ӳ���Э��ʧ�ܣ��Ͽ���ǰ����
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
		//���Ӳ���Э�̳ɹ�
		if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
       //���ܴ���;
    }
}

//���Ӳ���Э��ģ��������¼�������nrf_error�����˴�����룬ͨ��nrf_error���Է���������Ϣ
static void conn_params_error_handler(uint32_t nrf_error)
{
    //���������
	  APP_ERROR_HANDLER(nrf_error);
}


//���Ӳ���Э��ģ���ʼ��
static void conn_params_init(void)
{
    ret_code_t             err_code;
	  //�������Ӳ���Э��ģ���ʼ���ṹ��
    ble_conn_params_init_t cp_init;
    //����֮ǰ������
    memset(&cp_init, 0, sizeof(cp_init));
    //����ΪNULL����������ȡ���Ӳ���
    cp_init.p_conn_params                  = NULL;
	  //���ӻ�����֪ͨ���״η������Ӳ�����������֮���ʱ������Ϊ5��
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	  //ÿ�ε���sd_ble_gap_conn_param_update()�����������Ӳ������������֮��ļ��ʱ������Ϊ��30��
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	  //�������Ӳ���Э��ǰ�������Ӳ���Э�̵�����������Ϊ��3��
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	  //���Ӳ������´������¼���ʼ��ʱ
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	  //���Ӳ�������ʧ�ܲ��Ͽ�����
    cp_init.disconnect_on_fail             = false;
	  //ע�����Ӳ��������¼����
    cp_init.evt_handler                    = on_conn_params_evt;
	  //ע�����Ӳ������´����¼����
    cp_init.error_handler                  = conn_params_error_handler;
    //���ÿ⺯���������Ӳ������³�ʼ���ṹ��Ϊ�����������ʼ�����Ӳ���Э��ģ��
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

//�㲥�¼�������
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;
    //�жϹ㲥�¼�����
    switch (ble_adv_evt)
    {
        //���ٹ㲥�����¼������ٹ㲥�������������¼�
			  case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
			      //���ù㲥ָʾ��Ϊ���ڹ㲥��D1ָʾ����˸��
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        //�㲥IDLE�¼����㲥��ʱ���������¼�
        case BLE_ADV_EVT_IDLE:
					  //���ù㲥ָʾ��Ϊ�㲥ֹͣ��D1ָʾ��Ϩ��
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}
//��ȡƬ���¶ȴ�����
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
//�㲥��ʼ��
static void advertising_init(void)
{
    ret_code_t             err_code;

	  //����һ�����������¶�(Ƭ���¶ȴ�������ȡ���¶�)
    uint32_t temperature_data = temperature_data_get();
	
	  //����㲥��ʼ�����ýṹ�����
    ble_advertising_init_t init;
    //����֮ǰ������
    memset(&init, 0, sizeof(init));
    //�豸�������ͣ�ȫ��
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	  //�Ƿ������ۣ�����
    init.advdata.include_appearance      = false;
	  //Flag:һ��ɷ���ģʽ����֧��BR/EDR
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		
		//��ص�������UUID 0x180F
		service_data[0].service_uuid = BLE_UUID_BATTERY_SERVICE;
    service_data[0].data.size    = sizeof(battery_level);
    service_data[0].data.p_data  = &battery_level;
    //�¶ȷ���UUID 0x1809
    service_data[1].service_uuid = BLE_UUID_HEALTH_THERMOMETER_SERVICE;
    service_data[1].data.size    = sizeof(temperature_data);
    service_data[1].data.p_data  = (uint8_t *) &temperature_data;


    //�㲥�����м����������
	  init.advdata.p_service_data_array = service_data;
		//����������������Ϊ2
	  init.advdata.service_data_count = 2;
		
	  
	  //UUID�ŵ�ɨ����Ӧ����
	  init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;
	
    //���ù㲥ģʽΪ���ٹ㲥
    init.config.ble_adv_fast_enabled  = true;
	  //���ù㲥����͹㲥����ʱ��
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    //�㲥�¼��ص�����
    init.evt_handler = on_adv_evt;
    //��ʼ���㲥
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);
    //���ù㲥���ñ�ǡ�APP_BLE_CONN_CFG_TAG�����ڸ��ٹ㲥���õı�ǣ�����Ϊδ��Ԥ����һ���������ڽ�����SoftDevice�汾�У�
		//����ʹ��sd_ble_gap_adv_set_configure()�����µĹ㲥����
		//��ǰSoftDevice�汾��S132 V7.0.1�汾��֧�ֵ����㲥������Ϊ1�����APP_BLE_CONN_CFG_TAGֻ��д1��
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

//BLE�¼�������
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
    //�ж�BLE�¼����ͣ������¼�����ִ����Ӧ����
    switch (p_ble_evt->header.evt_id)
    {
        //�Ͽ������¼�
			  case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
				    
				    //��ӡ��ʾ��Ϣ
				    NRF_LOG_INFO("Disconnected.");
				    uart_reconfig();
				    //���ӶϿ�����������APP��ʱ��
            err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
            APP_ERROR_CHECK(err_code);
            break;
				
        //�����¼�
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
				    //����ָʾ��״̬Ϊ����״̬����ָʾ��D1����
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
				    //�������Ӿ��
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
				    //�����Ӿ��������Ŷ�д��ʵ����������Ŷ�д��ʵ���͸����ӹ��������������ж�����ӵ�ʱ��ͨ��������ͬ���Ŷ�д��ʵ�����ܷ��㵥�������������
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
				    //���ӽ�����ֹͣ���¹㲥����
				    //err_code = app_timer_stop(m_battery_timer_id);
				    APP_ERROR_CHECK(err_code);
						
            break;
				
        //PHY�����¼�
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
						//��ӦPHY���¹��
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
				//��ȫ���������¼�
				case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            //��֧�����
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
				 
				//ϵͳ���Է������ڵȴ���
				case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            //ϵͳ����û�д洢������ϵͳ����
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;
        //GATT�ͻ��˳�ʱ�¼�
        case BLE_GATTC_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Client Timeout.");
				    //�Ͽ���ǰ����
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        //GATT��������ʱ�¼�
        case BLE_GATTS_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Server Timeout.");
				    //�Ͽ���ǰ����
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}

//��ʼ��BLEЭ��ջ
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //����ʹ��SoftDevice���ú����л����sdk_config.h�ļ��е�Ƶʱ�ӵ����������õ�Ƶʱ��
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //���屣��Ӧ�ó���RAM��ʼ��ַ�ı���
    uint32_t ram_start = 0;
	  //ʹ��sdk_config.h�ļ���Ĭ�ϲ�������Э��ջ����ȡӦ�ó���RAM��ʼ��ַ�����浽����ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //ʹ��BLEЭ��ջ
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    //ע��BLE�¼��ص�����
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}
//��ʼ����Դ����ģ��
static void power_management_init(void)
{
    ret_code_t err_code;
	  //��ʼ����Դ����
    err_code = nrf_pwr_mgmt_init();
	  //��麯�����صĴ������
    APP_ERROR_CHECK(err_code);
}

//��ʼ��ָʾ��
static void leds_init(void)
{
    ret_code_t err_code;
	
    //��ʼ��BSPָʾ��
    err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}
//���¹㲥����
static void adv_updata(uint8_t bat_data,uint32_t temp_data)
{
	  ret_code_t err_code;
	  ble_advdata_t           adv_data; //�㲥����
    ble_advdata_t           sr_data;  //ɨ����Ӧ����
	
	
	  //��ص�������UUID 0x180F
		service_data[0].service_uuid = BLE_UUID_BATTERY_SERVICE;
    service_data[0].data.size    = sizeof(bat_data);
    service_data[0].data.p_data  = &bat_data;
    //�¶ȷ���UUID 0x1809
    service_data[1].service_uuid = BLE_UUID_HEALTH_THERMOMETER_SERVICE;
    service_data[1].data.size    = sizeof(temp_data);
    service_data[1].data.p_data  = (uint8_t *) &temp_data;

	  //�����㣬������
	  memset(&adv_data, 0, sizeof(adv_data));
	  memset(&sr_data, 0, sizeof(sr_data));
	  adv_data.name_type               = BLE_ADVDATA_FULL_NAME;
	  //�Ƿ������ۣ�����
    adv_data.include_appearance      = false;
	  //Flag:һ��ɷ���ģʽ����֧��BR/EDR
    adv_data.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		
		//�㲥�����м����������
	  adv_data.p_service_data_array = service_data;
		//����������������Ϊ2
	  adv_data.service_data_count = 2;
			
	  //UUID�ŵ�ɨ����Ӧ����
	  sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids  = m_adv_uuids;
	  //���¹㲥����
	  err_code = ble_advertising_advdata_update(&m_advertising, &adv_data, &sr_data);
		APP_ERROR_CHECK(err_code);
}
//SAADC�¼�������
void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
	  if (p_event->type == NRF_DRV_SAADC_EVT_DONE)//SAADC�������¼�
    {
			  nrf_saadc_value_t adc_result;
        uint16_t          batt_lvl_in_milli_volts;
        uint32_t          err_code;
         
        //���úû��棬Ϊ��һ�β���׼��
        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
        APP_ERROR_CHECK(err_code);
			
			  //��ȡ����ֵ
        adc_result = p_event->data.done.p_buffer[0];
				char buf[20];
				
				sprintf(buf,"{\"v\":%5d,\"u\":\"v\"}",adc_result);
				uint16_t len = strlen(buf);
			
			ble_voltage_data_send(&m_voltage,buf,&len,m_conn_handle);
			
        //������ֵת��Ϊ��ѹ����λmV
        batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result);
			  //��ѹת��Ϊ����
        battery_level = battery_level_in_percent(batt_lvl_in_milli_volts);
			
			  //����һ�����������¶�(Ƭ���¶ȴ�������ȡ���¶�)
        temperature_data = temperature_data_get();
			  //ֻ�е��¶Ȼ��ߵ�ص����ı�ʱ�Ÿ��¹㲥����
			  if((battery_level != battery_level_last) || (temperature_data != temperature_data_last))
				{
					//���¹㲥����
	        adv_updata(battery_level,temperature_data);
					//���浱ǰ���ݣ��Ա���һ�ζԱ�
					battery_level_last = battery_level;
					temperature_data_last = temperature_data;		
				}
    }
}
//SAADC��ʼ����ʹ��˫����
static void adc_configure(void)
{
    ret_code_t err_code = nrf_drv_saadc_init(NULL, saadc_event_handler);
    APP_ERROR_CHECK(err_code);

    nrf_saadc_channel_config_t config =
//        NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN2);
	NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_DIFFERENTIAL(NRF_SAADC_INPUT_AIN3, NRF_SAADC_INPUT_AIN2);
	  //��ʼ��SAADC��ע���¼��ص�����
    err_code = nrf_drv_saadc_channel_init(0, &config);
    APP_ERROR_CHECK(err_code);
    //���û���1��������1��ַ��ֵ��SAADC���������еĿ��ƿ�m_cb��һ������ָ��
    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
    APP_ERROR_CHECK(err_code);
    //���û���2��������1��ַ��ֵ��SAADC���������еĿ��ƿ�m_cb�Ķ�������ָ��
    err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
    APP_ERROR_CHECK(err_code);
}

//void diff_saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
//{
//	  if (p_event->type == NRF_DRV_SAADC_EVT_DONE)//SAADC�������¼�
//    {
//				nrf_saadc_value_t adc_result;
//        uint16_t          batt_lvl_in_milli_volts;
//        uint32_t          err_code;
//         
//        //���úû��棬Ϊ��һ�β���׼��
//        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
//        APP_ERROR_CHECK(err_code);
//			
//			  //��ȡ����ֵ
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
//	//���û���1��������1��ַ��ֵ��SAADC���������еĿ��ƿ�m_cb��һ������ָ��
//    err_code = nrf_drv_saadc_buffer_convert(&diff_adc_buf[0], 1);
//    APP_ERROR_CHECK(err_code);
//    //���û���2��������1��ַ��ֵ��SAADC���������еĿ��ƿ�m_cb�Ķ�������ָ��
//    err_code = nrf_drv_saadc_buffer_convert(&diff_adc_buf[1], 1);
//    APP_ERROR_CHECK(err_code);
//}

//APP��ʱ����ʱ�¼��ص��������ú���������һ��SAADC����
static void adv_updata_timeout_handler(void * p_context)
{
	  ret_code_t err_code;
	  //��ֹ���������棬ͬʱ�����صı���p_contextδʹ�ã�������Ӧ�ó�������ˡ����Ӧ�ó�����Ҫʹ��p_context������Ҫ���д���
	  UNUSED_PARAMETER(p_context);
	  //����һ��ADC����
	  err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}
//��ʼ��APP��ʱ��ģ��
static void timers_init(void)
{
    //��ʼ��APP��ʱ��ģ��
    ret_code_t err_code = app_timer_init();
	  //��鷵��ֵ
    APP_ERROR_CHECK(err_code);

    //����APP��ʱ������ز���APP��ʱ�������ڶ�ʱ�����������������У����ڵ�ز���
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                adv_updata_timeout_handler);
	
	  APP_ERROR_CHECK(err_code); 

}
//�����Ѿ������ĵ�APP��ʱ��
static void application_timers_start(void)
{
    ret_code_t err_code;

    //������ز���APP��ʱ��
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
	  //��麯�����صĴ������
    APP_ERROR_CHECK(err_code);

}
static void log_init(void)
{
    //��ʼ��log����ģ��
	  ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //����log����նˣ�����sdk_config.h�е�������������ն�ΪUART����RTT��
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//����״̬�����������û�й������־��������˯��ֱ����һ���¼���������ϵͳ
static void idle_state_handle(void)
{
    //��������log
	  if (NRF_LOG_PROCESS() == false)
    {
        //���е�Դ�����ú�����Ҫ�ŵ���ѭ������ִ��
			  nrf_pwr_mgmt_run();
    }
}
//�����㲥���ú������õ�ģʽ����͹㲥��ʼ�������õĹ㲥ģʽһ��
static void advertising_start(void)
{
   //ʹ�ù㲥��ʼ�������õĹ㲥ģʽ�����㲥
	 ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
	 //��麯�����صĴ������
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

//������
int main(void)
{
	//��ʼ��log����ģ��
	log_init();
	//��ʼ������
	//uart_config();
	button_config();
	//��ʼ��APP��ʱ��
	timers_init();
	//��ʹ��������ָʾ��
	leds_init();
	//��ʼ����Դ����
	power_management_init();
	//��ʼ��Э��ջ
	ble_stack_init();
	//��ʼ��SAADC
	adc_configure();
	//����GAP����
	gap_params_init();
	//��ʼ��GATT
	gatt_init();
	//��ʼ������
	services_init();
	//��ʼ���㲥
	advertising_init();	
	//���Ӳ���Э�̳�ʼ��
  conn_params_init();
	
	
  NRF_LOG_INFO("BLE Template example started."); 
  //����APP��ʱ��	
	application_timers_start();	
	//�����㲥
	advertising_start();
  //��ѭ��
	while(true)
	{
		//��������LOG�����е�Դ����
		idle_state_handle();
	}
}

