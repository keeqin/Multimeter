#include "my_ble_voltage.h"

#include "sdk_common.h"


#include <stdlib.h>
#include <string.h>
#include "app_error.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include "nrf_log.h"



uint32_t ble_voltage_data_send(ble_voltage_t * p_voltage,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle)
{
    ble_gatts_hvx_params_t     hvx_params;
    //验证p_uarts没有指向NULL
    VERIFY_PARAM_NOT_NULL(p_voltage);

    //如果连接句柄无效，表示没有和主机建立连接，返回NRF_ERROR_NOT_FOUND
	  if (conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    if (*p_length > 20)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    //设置之前先清零hvx_params
    memset(&hvx_params, 0, sizeof(hvx_params));
    //TX特征值句柄
    hvx_params.handle = p_voltage->tx_handles.value_handle;
		//发送的数据
    hvx_params.p_data = p_data;
		//发送的数据长度
    hvx_params.p_len  = p_length;
		//类型为通知
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    //发送TX特征值通知
    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

void ble_voltage_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    //检查参数是否有效
	  if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    //判断事件类型
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE://写事件
					  //处理写事件
				NRF_LOG_INFO("voltage-EVT_WRITE.");
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE://TX就绪事件
					  //处理TX就绪事件
				NRF_LOG_INFO("voltage-TX_COMPLETE.");
            break;

        default:
            break;
    }
}

//初始化adc
uint32_t ble_voltage_init(ble_voltage_t * p_voltage)
{
    ret_code_t            err_code;
	  //定义16位UUID结构体变量
    ble_uuid_t            ble_uuid;
	  //定义128位UUID结构体变量，并初始化为串口透传服务UUID基数
    ble_uuid128_t         nus_base_uuid = VOLTAGE_BASE_UUID;
	  //定义特征参数结构体变量
    ble_add_char_params_t add_char_params;
	

    //将自定义UUID基数添加到SoftDevice
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_voltage->uuid_type);
    VERIFY_SUCCESS(err_code);
    //UUID类型和数值赋值给ble_uuid变量
    ble_uuid.type = p_voltage->uuid_type;
    ble_uuid.uuid = BLE_UUID_VOLTAGE_SERVICE;

    //添加串口透传服务
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_voltage->service_handle);
    VERIFY_SUCCESS(err_code);
		
    /*---------------------以下代码添加TX特征--------------------*/
    //添加TX特征
		//配置参数之前先清零add_char_params
    memset(&add_char_params, 0, sizeof(add_char_params));
		//TX特征的UUID
    add_char_params.uuid              = BLE_UUID_VOLTAGE_TX_CHARACTERISTIC;
		//TX特征的UUID类型
    add_char_params.uuid_type         = p_voltage->uuid_type;
		//设置TX特征值的最大长度
    add_char_params.max_len           = 20;
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
    return characteristic_add(p_voltage->service_handle, &add_char_params, &p_voltage->tx_handles);
		/*---------------------添加TX特征-END------------------------*/
}
