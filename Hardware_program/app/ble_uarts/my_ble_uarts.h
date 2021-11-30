/****************************************Copyright (c)************************************************
**                                      [����ķ�Ƽ�]
**                                        IIKMSIK 
**                            �ٷ����̣�https://acmemcu.taobao.com
**                            �ٷ���̳��http://www.e930bbs.com                                 
**--------------File Info-----------------------------------------------------------------------------
** File          name:my_ble_uarts.h
** Last modified Date:          
** Last       Version:		   
** Descriptions      :����͸������ͷ�ļ�			
**---------------------------------------------------------------------------------------------------*/
#ifndef BLE_MY_UARTS_H__
#define BLE_MY_UARTS_H__
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

//���崮��͸������ʵ������ʵ�����2������
//1��������static���ʹ���͸������ṹ�������Ϊ����͸������ṹ��������ڴ�
//2��ע����BLE�¼������ߣ���ʹ�ô���͸������ģ����Խ���BLEЭ��ջ���¼����Ӷ�������ble_uarts_on_ble_evt()�¼��ص������д����Լ�����Ȥ���¼�
#define BLE_UARTS_DEF(_name, _uarts_max_clients)                                      \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_uarts_max_clients),                  \
                             sizeof(ble_uarts_client_context_t));   \
	  static ble_uarts_t _name =                                     \
	  {                                                             \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage) \
    };                                                            \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_uarts_on_ble_evt,                    \
                         &_name)

//���崮��͸������128λUUID����
#define UARTS_BASE_UUID      {{0x40, 0xE3, 0x4A, 0x1D, 0xC2, 0x5F, 0xB0, 0x9C, 0xB7, 0x47, 0xE6, 0x43, 0x00, 0x00, 0x53, 0x86}} 
//��������������16λUUID
#define BLE_UUID_UARTS_SERVICE 0x000A              //����͸������16λUUID
#define BLE_UUID_UARTS_TX_CHARACTERISTIC 0x000B    //TX����16λUUID           
#define BLE_UUID_UARTS_RX_CHARACTERISTIC 0x000C    //RX����16λUUID

//���崮��͸���¼����ͣ������û��Լ�����ģ���Ӧ�ó���ʹ��
typedef enum
{
    BLE_UARTS_EVT_RX_DATA,      //���յ��µ�����
    BLE_UARTS_EVT_TX_RDY,       //׼�����������Է���������
	  BLE_NUS_EVT_COMM_STARTED,   //֪ͨ�Ѿ�ʹ��
    BLE_NUS_EVT_COMM_STOPPED,   //֪ͨ�Ѿ���ֹ
} ble_uarts_evt_type_t;	
	
/* Forward declaration of the ble_nus_t type. */
typedef struct ble_uarts_s ble_uarts_t;


//����͸������BLE_NUS_EVT_RX_DATA�¼����ݽṹ�壬�ýṹ�����ڵ�BLE_NUS_EVT_RX_DATA����ʱ�����յ�������Ϣ���ݸ�������
typedef struct
{
    uint8_t const * p_data; //ָ���Ž������ݵĻ���
    uint16_t        length; //���յ����ݳ���
} ble_uarts_evt_rx_data_t;

//��¼�Զ��豸�Ƿ�ʹ����RX������֪ͨ
typedef struct
{
    bool is_notification_enabled; 
} ble_uarts_client_context_t;

//����͸�������¼��ṹ��
typedef struct
{
    ble_uarts_evt_type_t       type;        //�¼�����
    ble_uarts_t                * p_uarts;   //ָ�򴮿�͸��ʵ����ָ��
    uint16_t                   conn_handle; //���Ӿ��
    ble_uarts_client_context_t * p_link_ctx;//ָ��link context
    union
    {
        ble_uarts_evt_rx_data_t rx_data; //BLE_NUS_EVT_RX_DATA�¼�����
    } params;
} ble_uarts_evt_t;



//��������볤��
#define OPCODE_LENGTH        1
//����������
#define HANDLE_LENGTH        2

//������������ݳ��ȣ��ֽ�����
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_UARTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_UARTS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

//���庯��ָ������ble_uarts_data_handler_t
typedef void (* ble_uarts_data_handler_t) (ble_uarts_evt_t * p_evt);


//���ڷ����ʼ���ṹ��
typedef struct
{
    ble_uarts_data_handler_t data_handler; //����������ݵ��¼����
} ble_uarts_init_t;


//����͸������ṹ�壬��������Ҫ����Ϣ
struct ble_uarts_s
{
    uint8_t                         uuid_type;          //UUID����
    uint16_t                        service_handle;     //����͸������������Э��ջ�ṩ��
    ble_gatts_char_handles_t        tx_handles;         //TX�������
    ble_gatts_char_handles_t        rx_handles;         //RX�������
	  blcm_link_ctx_storage_t * const p_link_ctx_storage; //ָ��洢���е�ǰ���Ӽ���context�ġ�link context���ľ����ָ��
    ble_uarts_data_handler_t        data_handler;       //����������ݵ��¼����
};




void ble_uarts_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_uarts_init(ble_uarts_t * p_uarts, ble_uarts_init_t const * p_uarts_init);
uint32_t ble_uarts_data_send(ble_uarts_t * p_uarts,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);



#ifdef __cplusplus
}
#endif

#endif // BLE_MY_UARTS_H__


