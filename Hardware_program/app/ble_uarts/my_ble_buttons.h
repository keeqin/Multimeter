#ifndef BLE_MY_BUTTONS_H__
#define BLE_MY_BUTTONS_H__
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif


//定义串口透传服务实例，该实例完成2件事情
//1：定义了static类型串口透传服务结构体变量，为串口透传服务结构体分配了内存
//2：注册了BLE事件监视者，这使得串口透传程序模块可以接收BLE协议栈的事件，从而可以在ble_uarts_on_ble_evt()事件回调函数中处理自己感兴趣的事件
#define BLE_BUTTONS_DEF(_name)                                      \
    static ble_buttons_t _name;                                     \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_buttons_on_ble_evt,                    \
                         &_name)

//定义串口透传服务128位UUID基数
#define BUTTON_BASE_UUID      {{0x40, 0xE3, 0x4A, 0x1D, 0xC2, 0x5F, 0xB0, 0x9C, 0xB7, 0x47, 0xE6, 0x43, 0x00, 0x00, 0x53, 0x86}} 
//定义服务和特征的16位UUID
#define BLE_UUID_BUTTON_SERVICE 0x0003              //串口透传服务16位UUID
#define BLE_UUID_BUTTON_TX_CHARACTERISTIC 0x0004    //TX特征16位UUID           
#define BLE_UUID_BUTTON_RX_CHARACTERISTIC 0x0005    //RX特征16位UUID

	
/* Forward declaration of the ble_nus_t type. */
typedef struct ble_buttons_s ble_buttons_t;




//定义操作码长度
#define OPCODE_LENGTH        1
//定义句柄长度
#define HANDLE_LENGTH        2

//定义最大传输数据长度（字节数）
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_UARTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_UARTS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif





//串口透传服务结构体，包含所需要的信息
struct ble_buttons_s
{
    uint8_t                         uuid_type;          //UUID类型
    uint16_t                        service_handle;     //串口透传服务句柄（由协议栈提供）
    ble_gatts_char_handles_t        tx_handles;         //TX特征句柄
};


uint32_t ble_buttons_init(ble_buttons_t * p_buttons);
void ble_buttons_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_buttons_data_send(ble_buttons_t * p_buttons,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);


#ifdef __cplusplus
}
#endif

#endif // BLE_MY_BUTTONS_H__


