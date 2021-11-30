
#include "sdk_common.h"
#include "my_ble_buttons.h"


#include <stdlib.h>
#include <string.h>
#include "app_error.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include "nrf_log.h"



#define BLE_UARTS_MAX_RX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN //RX������󳤶ȣ��ֽ�����
#define BLE_UARTS_MAX_TX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN //TX������󳤶ȣ��ֽ�����



//����͸������BLE�¼������ߵ��¼��ص�����
void ble_buttons_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    //�������Ƿ���Ч
	  if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    //����һ������͸���ṹ��ָ�벢ָ�򴮿�͸���ṹ��
//    ble_uarts_t * p_uarts = (ble_uarts_t *)p_context;
    //�ж��¼�����
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE://д�¼�
					  //����д�¼�
				NRF_LOG_INFO("EVT_WRITE.");
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE://TX�����¼�
					  //����TX�����¼�
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
    //��֤p_uartsû��ָ��NULL
    VERIFY_PARAM_NOT_NULL(p_buttons);

    //������Ӿ����Ч����ʾû�к������������ӣ�����NRF_ERROR_NOT_FOUND
	  if (conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_NOT_FOUND;
    }

    if (*p_length > BLE_UARTS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    //����֮ǰ������hvx_params
    memset(&hvx_params, 0, sizeof(hvx_params));
    //TX����ֵ���
    hvx_params.handle = p_buttons->tx_handles.value_handle;
		//���͵�����
    hvx_params.p_data = p_data;
		//���͵����ݳ���
    hvx_params.p_len  = p_length;
		//����Ϊ֪ͨ
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    //����TX����ֵ֪ͨ
    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

//��ʼ������͸������
uint32_t ble_buttons_init(ble_buttons_t * p_buttons)
{
    ret_code_t            err_code;
	  //����16λUUID�ṹ�����
    ble_uuid_t            ble_uuid;
	  //����128λUUID�ṹ�����������ʼ��Ϊ����͸������UUID����
    ble_uuid128_t         nus_base_uuid = BUTTON_BASE_UUID;
	  //�������������ṹ�����
    ble_add_char_params_t add_char_params;
	

    //���Զ���UUID������ӵ�SoftDevice
    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_buttons->uuid_type);
    VERIFY_SUCCESS(err_code);
    //UUID���ͺ���ֵ��ֵ��ble_uuid����
    ble_uuid.type = p_buttons->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_SERVICE;

    //��Ӵ���͸������
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_buttons->service_handle);
    VERIFY_SUCCESS(err_code);
		
    /*---------------------���´������TX����--------------------*/
    //���TX����
		//���ò���֮ǰ������add_char_params
    memset(&add_char_params, 0, sizeof(add_char_params));
		//TX������UUID
    add_char_params.uuid              = BLE_UUID_BUTTON_TX_CHARACTERISTIC;
		//TX������UUID����
    add_char_params.uuid_type         = p_buttons->uuid_type;
		//����TX����ֵ����󳤶�
    add_char_params.max_len           = BLE_UARTS_MAX_TX_CHAR_LEN;
		//����TX����ֵ�ĳ�ʼ����
    add_char_params.init_len          = sizeof(uint8_t);
		//����TX������ֵ����Ϊ�ɱ䳤��
    add_char_params.is_var_len        = true;
		//����TX���������ʣ�֧��֪ͨ
    add_char_params.char_props.notify = 1;
    //���ö�/дRX����ֵ�İ�ȫ�����ް�ȫ��
    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;
    //Ϊ����͸���������TX����
    return characteristic_add(p_buttons->service_handle, &add_char_params, &p_buttons->tx_handles);
		/*---------------------���TX����-END------------------------*/
}
