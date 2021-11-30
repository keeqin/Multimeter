#ifndef BLE_MY_VOLTAGE_H__
#define BLE_MY_VOLTAGE_H__
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif


#define BLE_VOLTAGE_DEF(_name)                                      \
    static ble_voltage_t _name;                                     \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_voltage_on_ble_evt,                    \
                         &_name)

//定义串口透传服务128位UUID基数
#define VOLTAGE_BASE_UUID      {{0x40, 0xE3, 0x4A, 0x1D, 0xC2, 0x5F, 0xB0, 0x9C, 0xB7, 0x47, 0xE6, 0x43, 0x00, 0x00, 0x53, 0x55}} 
//定义服务和特征的16位UUID
#define BLE_UUID_VOLTAGE_SERVICE 0x0A03              //串口透传服务16位UUID
#define BLE_UUID_VOLTAGE_TX_CHARACTERISTIC 0x0A04    //TX特征16位UUID




typedef struct ble_voltage_s ble_voltage_t;

struct ble_voltage_s
{
    uint8_t                         uuid_type;          //UUID类型
    uint16_t                        service_handle;     //串口透传服务句柄（由协议栈提供）
    ble_gatts_char_handles_t        tx_handles;         //TX特征句柄
};

void ble_voltage_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_voltage_init(ble_voltage_t * p_voltage);
uint32_t ble_voltage_data_send(ble_voltage_t * p_voltage,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);



#endif

