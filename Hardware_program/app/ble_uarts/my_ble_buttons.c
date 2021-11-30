
#include "sdk_common.h"
#include "my_ble_buttons.h"


#include <stdlib.h>
#include <string.h>
#include "app_error.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include "nrf_log.h"



#define BLE_UARTS_MAX_RX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN //RX特征最大长度（字节数）
#define BLE_UARTS_MAX_TX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN //TX特征最大长度（字节数）



//串口透传服务BLE事件监视者的事件回调函数
void ble_buttons_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    //检查参数是否有效
	  if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    //定义一个串口透传结构体指针并指向串口透传结构体
//    ble_uarts_t * p_uarts = (ble_uarts_t *)p_context;
    //判断事件类型
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE://写事件
					  //处理写事件
				NRF_LOG_INFO("EVT_WRITE.");
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE://TX就绪事件
					  //处理TX就绪事件
				NRF_LOG_INFO("TX_COMPLETE.");
            break;

        default:
            break;
    }
}

uint32_t ble_buttons_data_send(ble_buttons_t * p_buttons,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle)
{
    ble_gatts_hvx_params_t     hvx_params;
    //验证p_uarts没有指向NULL
    VERIFY_PARAM_NOT_NULL(p_buttons);

    //如果连接句柄无效，表示没有和主机建立连接，返回NRF_ERROR_NOT_FOUND
	  if (conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    if (*p_length > BLE_UARTS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    //设置之前先清零hvx_params
    memset(&hvx_params, 0, sizeof(hvx_params));
    //TX特征值句柄
    hvx_params.handle = p_buttons->tx_handles.value_handle;
		//发送的数据
    hvx_params.p_data = p_data;
		//发送的数据长度
    hvx_params.p_len  = p_length;
		//类型为通知
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    //发送TX特征值通知
    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

//初始化串口透传服务
uint32_t ble_buttons_init(ble_buttons_t * p_buttons)
{
    ret_code_t            err_code;
	  //定义16位UUID结构体变量
    ble_uuid_t            ble_uuid;
	  //定义128位UUID结构体变量，并初始化为串口透传服务UUID基数
    ble_uuid128_t         nus_base_uuid = BUTTON_BASE_UUID;
	  //定义特征参数结构体变量
    ble_add_char_params_t add_char_params;
	

    //将自定义UUID基数添加到SoftDevice
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_buttons->uuid_type);
    VERIFY_SUCCESS(err_code);
    //UUID类型和数值赋值给ble_uuid变量
    ble_uuid.type = p_buttons->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_SERVICE;

    //添加串口透传服务
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_buttons->service_handle);
    VERIFY_SUCCESS(err_code);
		
    /*---------------------以下代码添加TX特征--------------------*/
    //添加TX特征
		//配置参数之前先清零add_char_params
    memset(&add_char_params, 0, sizeof(add_char_params));
		//TX特征的UUID
    add_char_params.uuid              = BLE_UUID_BUTTON_TX_CHARACTERISTIC;
		//TX特征的UUID类型
    add_char_params.uuid_type         = p_buttons->uuid_type;
		//设置TX特征值的最大长度
    add_char_params.max_len           = BLE_UARTS_MAX_TX_CHAR_LEN;
		//设置TX特征值的初始长度
    add_char_params.init_len          = sizeof(uint8_t);
		//设置TX的特征值长度为可变长度
    add_char_params.is_var_len        = true;
		//设置TX特征的性质：支持通知
    add_char_params.char_props.notify = 1;
    //设置读/写RX特征值的安全需求：无安全性
    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;
    //为串口透传服务添加TX特征
    return characteristic_add(p_buttons->service_handle, &add_char_params, &p_buttons->tx_handles);
		/*---------------------添加TX特征-END------------------------*/
}
