#include <string.h>
#include <ch32x035.h>
#include "ch32x035_usb.h"
#include "ch32x035_gpio.h"
#include "ch32x035_rcc.h"
#include "usb_device.h"
#include "usb_protocol.h"
#include "system_dfu.h"
#include "system_status.h"
#include "fan_controller.h"
#if FEATURE_USB_RAM_TRACE_DIAG
#include "usb_trace.h"
#endif

#define USB_EP0_SIZE 64U
#define USB_EP1_IN 0x81U
#define USB_EP2_OUT 0x02U
#define USB_REQ_GET_STATUS 0x00U
#define USB_REQ_CLEAR_FEATURE 0x01U
#define USB_REQ_SET_ADDRESS 0x05U
#define USB_REQ_GET_DESCRIPTOR 0x06U
#define USB_REQ_GET_CONFIGURATION 0x08U
#define USB_REQ_SET_CONFIGURATION 0x09U
#define USB_REQ_GET_INTERFACE 0x0AU
#define USB_REQ_SET_INTERFACE 0x0BU
#define USB_REQ_TYPE_STANDARD 0x00U
#define USB_REQ_TYPE_CLASS 0x20U
#define USB_REQ_DIR_IN 0x80U
#define USB_DESC_DEVICE 0x01U
#define USB_DESC_CONFIGURATION 0x02U
#define USB_DESC_STRING 0x03U
#define USB_DESC_HID 0x21U
#define USB_DESC_REPORT 0x22U
#define USB_HID_GET_REPORT 0x01U
#define USB_HID_GET_IDLE 0x02U
#define USB_HID_GET_PROTOCOL 0x03U
#define USB_HID_SET_IDLE 0x0AU
#define USB_HID_SET_PROTOCOL 0x0BU
#define USB_IOEN 0x00000080U
#define USB_PHY_V33 0x00000040U
#define UDP_PUE_MASK 0x0000000CU
#define UDP_PUE_1K5 0x0000000CU
#define UDM_PUE_MASK 0x00000003U

/* Development-only WCH VID/PID; not a CheeseCool commercial identity. */
#define USB_APP_VID 0x1A86U
#define USB_APP_PID 0xFE01U

__attribute__((aligned(4))) static uint8_t ep0_buf[USB_EP0_SIZE];
__attribute__((aligned(4))) static uint8_t ep1_in_buf[USB_REPORT_SIZE];
__attribute__((aligned(4))) static uint8_t ep2_out_buf[USB_REPORT_SIZE];
static uint8_t pending_out[USB_REPORT_SIZE];
static uint8_t tx_report[USB_REPORT_SIZE];
static volatile bool configured;
static volatile bool out_pending;
static volatile bool tx_busy;
static volatile bool tx_done;
static bool dfu_reset_waiting;
static uint32_t dfu_reset_due_ms;
static volatile uint8_t usb_address;
static volatile uint8_t usb_configuration;
static volatile uint8_t protocol_mode;
static volatile uint8_t idle_rate;
static uint32_t diag_start_ms;
static const uint8_t *control_ptr;
static uint16_t control_remaining;
static uint8_t control_request_code;
#if FEATURE_USB_RAM_TRACE_DIAG
static bool usb_ram_trace_collecting;
static uint32_t usb_ram_trace_started_ms;
#endif

#if FEATURE_USB_ENUM_TRACE
enum {
    USB_DEVICE_STATE_DETACHED = 0,
    USB_DEVICE_STATE_WAIT_ATTACH,
    USB_DEVICE_STATE_ATTACHED
};
static uint8_t usb_device_state;
static uint32_t usb_attach_started_ms;
#endif

#if FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG
enum {
    ATTACH_DIAG_PREPARE = 0,
    ATTACH_DIAG_DETACH_WAIT,
    ATTACH_DIAG_ATTACH_WAIT,
    ATTACH_DIAG_RESULT_SLOW,
    ATTACH_DIAG_RESULT_FAST,
    ATTACH_DIAG_RESULT_HOLD
};
static uint8_t attach_diag_phase;
static uint32_t attach_diag_phase_ms;
static uint8_t attach_diag_pulse_index;
static uint8_t attach_diag_pulse_total;
static volatile bool attach_diag_result_seen;
static volatile bool attach_diag_setup_seen;
static bool attach_diag_hold_fast;
static bool attach_diag_abnormal;
volatile bool usb_bus_reset_seen;
volatile bool usb_setup_seen;
volatile bool usb_get_device_descriptor_seen;
volatile bool usb_device_descriptor_response_started;
volatile bool usb_device_descriptor_in_complete;
volatile bool usb_set_address_seen;
volatile bool usb_address_applied;
volatile uint8_t usb_set_address_value;
volatile uint8_t usb_address_applied_value;
static bool device_descriptor_transfer_active;
volatile bool usb_get_config_descriptor_seen;
volatile bool usb_config_descriptor_in_complete;
volatile bool usb_set_configuration_seen;
volatile bool usb_configuration_applied;
static bool configuration_descriptor_transfer_active;
static uint16_t configuration_request_wLength;
volatile bool config_in_diag_a_seen;
volatile bool config_in_diag_b_selected;
volatile bool config_in_diag_c_dma_ready;
volatile bool config_in_diag_d_tx_armed;
volatile bool config_in_diag_e_in_token_seen;
volatile bool config_in_diag_f_in_complete;
volatile bool config_in_diag_first_seen;
volatile uint8_t config_in_diag_first_bmRequestType;
volatile uint8_t config_in_diag_first_bRequest;
volatile uint16_t config_in_diag_first_wValue;
volatile uint16_t config_in_diag_first_wIndex;
volatile uint16_t config_in_diag_first_wLength;
volatile uint32_t config_in_diag_descriptor_ptr;
volatile uint16_t config_in_diag_descriptor_length;
volatile uint16_t config_in_diag_send_length;
volatile uint32_t config_in_diag_ep0_data;
volatile uint32_t config_in_diag_ep0_dma;
volatile uint16_t config_in_diag_t_len;
volatile uint16_t config_in_diag_ctrl_h;
static bool config_in_diag_active;
static bool attach_diag_zero_event;
volatile bool config_runtime_seen;
volatile bool config_runtime_selected;
volatile uint8_t config_runtime_bmRequestType;
volatile uint8_t config_runtime_bRequest;
volatile uint16_t config_runtime_wValue;
volatile uint16_t config_runtime_wIndex;
volatile uint16_t config_runtime_wLength;
volatile uint16_t config_runtime_available;
volatile uint16_t config_runtime_remaining;
volatile uint16_t config_runtime_t_len;
volatile uint16_t config_runtime_ctrl_h;
volatile bool config_runtime_ep0_in_seen;
volatile uint8_t config_runtime_int_st;
volatile uint16_t config_runtime_t_len_at_in;
volatile uint16_t config_runtime_ctrl_h_at_in;
static bool config_runtime_active;
static bool config_runtime_output_started;
static bool config_runtime_dfu_due;
static uint8_t config_runtime_phase;
static uint32_t config_runtime_phase_ms;
static uint8_t config_runtime_pulse_index;
static uint8_t config_runtime_pulse_total;
static uint8_t config_runtime_tlen_pulses;
static uint8_t config_runtime_in_pulses;
#if FEATURE_USB_BINARY_WLENGTH9_DIAG
volatile bool config_request_seen;
volatile uint16_t first_config_wLength;
static bool binary_wlength9_result_started;
static bool binary_wlength9_dfu_due;
static uint32_t binary_wlength9_result_ms;
static bool binary_wlength9_yes;
#endif
#if FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
volatile bool config_seen_request;
static bool binary_config_seen_result_started;
static bool binary_config_seen_dfu_due;
static uint32_t binary_config_seen_result_ms;
static bool binary_config_seen_yes;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
volatile bool address_applied;
volatile bool post_address_setup_seen;
volatile bool post_address_bus_reset_seen;
static bool binary_post_address_result_started;
static bool binary_post_address_dfu_due;
static uint32_t binary_post_address_result_ms;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
volatile bool address_applied;
volatile bool first_post_address_setup_captured;
volatile uint8_t first_post_address_bmRequestType;
volatile uint8_t first_post_address_bRequest;
volatile uint16_t first_post_address_wValue;
volatile uint16_t first_post_address_wIndex;
volatile uint16_t first_post_address_wLength;
static bool binary_post_address_get_result_started;
static bool binary_post_address_get_dfu_due;
static uint32_t binary_post_address_get_result_ms;
static bool binary_post_address_get_yes;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
volatile bool address_applied_config_type;
volatile bool first_post_address_config_setup_captured;
volatile uint8_t first_post_address_config_bmRequestType;
volatile uint8_t first_post_address_config_bRequest;
volatile uint16_t first_post_address_config_wValue;
volatile uint16_t first_post_address_config_wIndex;
volatile uint16_t first_post_address_config_wLength;
static bool binary_post_address_config_result_started;
static bool binary_post_address_config_dfu_due;
static uint32_t binary_post_address_config_result_ms;
static bool binary_post_address_config_yes;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
volatile bool address_applied_string_type;
volatile bool first_post_address_string_setup_captured;
volatile uint8_t first_post_address_string_bmRequestType;
volatile uint8_t first_post_address_string_bRequest;
volatile uint16_t first_post_address_string_wValue;
volatile uint16_t first_post_address_string_wIndex;
volatile uint16_t first_post_address_string_wLength;
static bool binary_post_address_string_result_started;
static bool binary_post_address_string_dfu_due;
static uint32_t binary_post_address_string_result_ms;
static bool binary_post_address_string_yes;
#endif
volatile bool usb_ep0_rearmed_after_address;
volatile uint16_t usb_ep0_ctrl_after_address;
volatile bool usb_post_address_setup_seen;
volatile uint8_t post_address_bRequest;
volatile uint8_t post_address_bmRequestType;
volatile uint16_t post_address_wValue;
volatile uint16_t post_address_wIndex;
volatile uint16_t post_address_wLength;
volatile bool usb_config_length_request_seen;
volatile bool usb_config_length_short_in_complete;
volatile bool usb_config_length_long_request_seen;
volatile bool usb_config_length_long_in_complete;
volatile bool usb_config_length_set_configuration_seen;
volatile uint8_t usb_config_length_request_count;
volatile uint8_t usb_config_length_first_bmRequestType;
volatile uint8_t usb_config_length_first_bRequest;
volatile uint16_t usb_config_length_first_wValue;
volatile uint16_t usb_config_length_first_wIndex;
volatile uint16_t usb_config_length_first_wLength;
volatile uint8_t usb_config_length_second_bmRequestType;
volatile uint8_t usb_config_length_second_bRequest;
volatile uint16_t usb_config_length_second_wValue;
volatile uint16_t usb_config_length_second_wIndex;
volatile uint16_t usb_config_length_second_wLength;
#endif

enum {
    TRACE_BOOT = 1,
    TRACE_BUS_RESET,
    TRACE_SETUP_RECEIVED,
    TRACE_GET_DEVICE_DESCRIPTOR,
    TRACE_SET_ADDRESS,
    TRACE_GET_CONFIGURATION_DESCRIPTOR,
    TRACE_GET_STRING_DESCRIPTOR,
    TRACE_SET_CONFIGURATION,
    TRACE_HID_READY
};

volatile uint8_t usb_enum_trace_stage;
volatile uint8_t final_trace_stage;
static volatile uint8_t usb_enum_trace_pending_stage;
static volatile bool usb_enum_trace_config_complete;
static volatile bool usb_enum_trace_frozen;
static uint32_t usb_enum_trace_start_ms;
static uint32_t usb_enum_trace_phase_ms;
static uint8_t usb_enum_trace_pulse_index;
static uint8_t usb_enum_trace_phase;
volatile uint32_t usb_bus_reset_count;
volatile uint32_t usb_setup_count;
volatile uint32_t usb_get_device_desc_count;
volatile uint32_t usb_set_address_count;
volatile uint32_t usb_get_config_desc_count;
volatile uint32_t usb_get_string_desc_count;
volatile uint32_t usb_set_configuration_count;

enum {
    TRACE_PHASE_COLLECT = 0,
    TRACE_PHASE_MARKER,
    TRACE_PHASE_SLOW,
    TRACE_PHASE_FAST,
    TRACE_PHASE_HOLD
};

static void usb_enum_trace_request(uint8_t stage)
{
#if FEATURE_USB_ENUM_TRACE
    if (stage > usb_enum_trace_pending_stage) usb_enum_trace_pending_stage = stage;
#else
    (void)stage;
#endif
}

static const uint8_t device_descriptor[] = {
    0x12,0x01,0x00,0x02,0x00,0x00,0x00,USB_EP0_SIZE,
    (uint8_t)USB_APP_VID,(uint8_t)(USB_APP_VID>>8),
    (uint8_t)USB_APP_PID,(uint8_t)(USB_APP_PID>>8),
    0x00,0x01,0x01,0x02,0x03,0x01
};
static const uint8_t report_descriptor[] = {
    0x06,0x00,0xFF,0x09,0x01,0xA1,0x01,0x15,0x00,0x26,0xFF,0x00,
    0x75,0x08,0x95,USB_REPORT_SIZE,0x09,0x02,0x81,0x02,
    0x09,0x03,0x91,0x02,0xC0
};
static const uint8_t configuration_descriptor[] = {
    0x09,0x02,0x29,0x00,0x01,0x01,0x00,0x80,0x32,
    0x09,0x04,0x00,0x00,0x02,0x03,0x00,0x00,0x04,
    0x09,0x21,0x11,0x01,0x00,0x01,0x22,(uint8_t)sizeof(report_descriptor),(uint8_t)(sizeof(report_descriptor)>>8),
    0x07,0x05,USB_EP1_IN,0x03,USB_REPORT_SIZE,0x00,0x01,
    0x07,0x05,USB_EP2_OUT,0x03,USB_REPORT_SIZE,0x00,0x01
};
static const uint8_t lang_descriptor[] = {0x04,0x03,0x09,0x04};

static uint16_t string_descriptor(const char *s)
{
    uint16_t n=0;
    while (s[n] && (2U+2U*(n+1U)) <= sizeof(ep0_buf)) n++;
    ep0_buf[0]=(uint8_t)(2U+2U*n); ep0_buf[1]=USB_DESC_STRING;
    for (uint16_t i=0;i<n;i++) { ep0_buf[2U+2U*i]=(uint8_t)s[i]; ep0_buf[3U+2U*i]=0; }
    return ep0_buf[0];
}

static void endpoint_init(void)
{
    USBFSD->UEP0_DMA=(uint32_t)ep0_buf;
    USBFSD->UEP0_CTRL_H=USBFS_UEP_R_RES_ACK|USBFS_UEP_T_RES_NAK;
    USBFSD->UEP4_1_MOD=USBFS_UEP1_TX_EN;
    USBFSD->UEP2_3_MOD=USBFS_UEP2_RX_EN;
    USBFSD->UEP1_DMA=(uint32_t)ep1_in_buf;
    USBFSD->UEP2_DMA=(uint32_t)ep2_out_buf;
    USBFSD->UEP1_TX_LEN=0;
    USBFSD->UEP1_CTRL_H=USBFS_UEP_T_RES_NAK;
    USBFSD->UEP2_CTRL_H=USBFS_UEP_R_RES_ACK;
}

static void usb_enable_clocks(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
}

static void usb_gpio_init(void)
{
    GPIO_InitTypeDef gi={0};
    gi.GPIO_Pin=GPIO_Pin_16; gi.GPIO_Mode=GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOC,&gi);
    gi.GPIO_Pin=GPIO_Pin_17; gi.GPIO_Mode=GPIO_Mode_IPU; GPIO_Init(GPIOC,&gi);
}

static void usb_attach(void)
{
    usb_gpio_init();
    AFIO->CTLR=(AFIO->CTLR&~(UDP_PUE_MASK|UDM_PUE_MASK))|USB_PHY_V33|UDP_PUE_1K5|USB_IOEN;
    USBFSD->BASE_CTRL=0; endpoint_init(); USBFSD->DEV_ADDR=0;
    USBFSD->BASE_CTRL=USBFS_UC_DEV_PU_EN|USBFS_UC_INT_BUSY|USBFS_UC_DMA_EN;
    USBFSD->INT_FG=0xFF; USBFSD->UDEV_CTRL=USBFS_UD_PD_DIS|USBFS_UD_PORT_EN;
    USBFSD->INT_EN=USBFS_UIE_SUSPEND|USBFS_UIE_BUS_RST|USBFS_UIE_TRANSFER;
    NVIC_EnableIRQ(USBFS_IRQn);
}

#if FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG
void usb_attach_diag_start(void)
{
#if FEATURE_USB_RAM_TRACE_DIAG
    usb_trace_init();
    usb_ram_trace_collecting=false;
    usb_ram_trace_started_ms=0;
#endif
    usb_bus_reset_seen=false;
    usb_setup_seen=false;
    usb_get_device_descriptor_seen=false;
    usb_device_descriptor_response_started=false;
    usb_device_descriptor_in_complete=false;
    usb_set_address_seen=false;
    usb_address_applied=false;
    usb_set_address_value=0;
    usb_address_applied_value=0;
    usb_get_config_descriptor_seen=false;
    usb_config_descriptor_in_complete=false;
    usb_set_configuration_seen=false;
    usb_configuration_applied=false;
    usb_ep0_rearmed_after_address=false;
    usb_ep0_ctrl_after_address=0;
    usb_post_address_setup_seen=false;
    post_address_bRequest=0;
    post_address_bmRequestType=0;
    post_address_wValue=0;
    post_address_wIndex=0;
    post_address_wLength=0;
    usb_config_length_request_seen=false;
    usb_config_length_short_in_complete=false;
    usb_config_length_long_request_seen=false;
    usb_config_length_long_in_complete=false;
    usb_config_length_set_configuration_seen=false;
    usb_config_length_request_count=0;
    usb_config_length_first_bmRequestType=0;
    usb_config_length_first_bRequest=0;
    usb_config_length_first_wValue=0;
    usb_config_length_first_wIndex=0;
    usb_config_length_first_wLength=0;
    usb_config_length_second_bmRequestType=0;
    usb_config_length_second_bRequest=0;
    usb_config_length_second_wValue=0;
    usb_config_length_second_wIndex=0;
    usb_config_length_second_wLength=0;
    configuration_request_wLength=0;
    config_in_diag_a_seen=false;
    config_in_diag_b_selected=false;
    config_in_diag_c_dma_ready=false;
    config_in_diag_d_tx_armed=false;
    config_in_diag_e_in_token_seen=false;
    config_in_diag_f_in_complete=false;
    config_in_diag_first_seen=false;
    config_in_diag_first_bmRequestType=0;
    config_in_diag_first_bRequest=0;
    config_in_diag_first_wValue=0;
    config_in_diag_first_wIndex=0;
    config_in_diag_first_wLength=0;
    config_in_diag_descriptor_ptr=0;
    config_in_diag_descriptor_length=0;
    config_in_diag_send_length=0;
    config_in_diag_ep0_data=0;
    config_in_diag_ep0_dma=0;
    config_in_diag_t_len=0;
    config_in_diag_ctrl_h=0;
    config_in_diag_active=false;
    attach_diag_zero_event=false;
    config_runtime_seen=false;
    config_runtime_selected=false;
    config_runtime_bmRequestType=0;
    config_runtime_bRequest=0;
    config_runtime_wValue=0;
    config_runtime_wIndex=0;
    config_runtime_wLength=0;
    config_runtime_available=0;
    config_runtime_remaining=0;
    config_runtime_t_len=0;
    config_runtime_ctrl_h=0;
    config_runtime_ep0_in_seen=false;
    config_runtime_int_st=0;
    config_runtime_t_len_at_in=0;
    config_runtime_ctrl_h_at_in=0;
    config_runtime_active=false;
    config_runtime_output_started=false;
    config_runtime_dfu_due=false;
    config_runtime_phase=0;
    config_runtime_phase_ms=0;
    config_runtime_pulse_index=0;
    config_runtime_pulse_total=0;
    config_runtime_tlen_pulses=0;
    config_runtime_in_pulses=0;
#if FEATURE_USB_BINARY_WLENGTH9_DIAG
    config_request_seen=false;
    first_config_wLength=0;
    binary_wlength9_result_started=false;
    binary_wlength9_dfu_due=false;
    binary_wlength9_result_ms=0;
    binary_wlength9_yes=false;
#endif
#if FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
    config_seen_request=false;
    binary_config_seen_result_started=false;
    binary_config_seen_dfu_due=false;
    binary_config_seen_result_ms=0;
    binary_config_seen_yes=false;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
    address_applied=false;
    post_address_setup_seen=false;
    post_address_bus_reset_seen=false;
    binary_post_address_result_started=false;
    binary_post_address_dfu_due=false;
    binary_post_address_result_ms=0;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
    address_applied=false;
    first_post_address_setup_captured=false;
    first_post_address_bmRequestType=0;
    first_post_address_bRequest=0;
    first_post_address_wValue=0;
    first_post_address_wIndex=0;
    first_post_address_wLength=0;
    binary_post_address_get_result_started=false;
    binary_post_address_get_dfu_due=false;
    binary_post_address_get_result_ms=0;
    binary_post_address_get_yes=false;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
    address_applied_config_type=false;
    first_post_address_config_setup_captured=false;
    first_post_address_config_bmRequestType=0;
    first_post_address_config_bRequest=0;
    first_post_address_config_wValue=0;
    first_post_address_config_wIndex=0;
    first_post_address_config_wLength=0;
    binary_post_address_config_result_started=false;
    binary_post_address_config_dfu_due=false;
    binary_post_address_config_result_ms=0;
    binary_post_address_config_yes=false;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
    address_applied_string_type=false;
    first_post_address_string_setup_captured=false;
    first_post_address_string_bmRequestType=0;
    first_post_address_string_bRequest=0;
    first_post_address_string_wValue=0;
    first_post_address_string_wIndex=0;
    first_post_address_string_wLength=0;
    binary_post_address_string_result_started=false;
    binary_post_address_string_dfu_due=false;
    binary_post_address_string_result_ms=0;
    binary_post_address_string_yes=false;
#endif
    attach_diag_result_seen=false;
    attach_diag_setup_seen=false;
    attach_diag_hold_fast=false;
    attach_diag_abnormal=false;
    attach_diag_pulse_index=0;
    attach_diag_pulse_total=0;
    attach_diag_phase=ATTACH_DIAG_PREPARE;
    attach_diag_phase_ms=system_millis();
    fan_controller_enable();
    fan_controller_set_duty(100U);
}

#if FEATURE_USB_CONFIG_RUNTIME_DIAG
enum {
    CONFIG_RUNTIME_WAIT = 0,
    CONFIG_RUNTIME_MARK_SLOW,
    CONFIG_RUNTIME_MARK_FAST,
    CONFIG_RUNTIME_MARK_TAIL,
    CONFIG_RUNTIME_WL_MISSING,
    CONFIG_RUNTIME_WL_FAST,
    CONFIG_RUNTIME_WL_SLOW,
    CONFIG_RUNTIME_SEP_TLEN_FAST,
    CONFIG_RUNTIME_SEP_TLEN_SLOW,
    CONFIG_RUNTIME_TLEN_FAST,
    CONFIG_RUNTIME_TLEN_SLOW,
    CONFIG_RUNTIME_SEP_IN_FAST,
    CONFIG_RUNTIME_SEP_IN_SLOW,
    CONFIG_RUNTIME_IN_FAST,
    CONFIG_RUNTIME_IN_SLOW,
    CONFIG_RUNTIME_FINAL,
    CONFIG_RUNTIME_DONE
};

static void config_runtime_diag_begin(uint32_t now)
{
    config_runtime_output_started=true;
    config_runtime_phase=CONFIG_RUNTIME_MARK_SLOW;
    config_runtime_phase_ms=now;
    config_runtime_pulse_index=0;
    fan_controller_set_duty(0U);
}

static uint8_t config_runtime_wlength_pulses(void)
{
    if (!config_runtime_seen) return 0U;
    if (config_runtime_wLength==9U) return 1U;
    if (config_runtime_wLength==41U) return 2U;
    if (config_runtime_wLength==64U) return 3U;
    if (config_runtime_wLength==255U) return 4U;
    return 5U;
}

static uint8_t config_runtime_tlen_class(void)
{
    if (config_runtime_t_len==0U) return 1U;
    if (config_runtime_t_len==9U) return 2U;
    if (config_runtime_t_len==41U) return 3U;
    return 4U;
}

static void config_runtime_diag_task(uint32_t now)
{
    uint32_t elapsed=(uint32_t)(now-config_runtime_phase_ms);
    switch (config_runtime_phase) {
    case CONFIG_RUNTIME_MARK_SLOW:
        if (elapsed>=4000UL) { config_runtime_phase=CONFIG_RUNTIME_MARK_FAST; config_runtime_phase_ms=now; fan_controller_set_duty(100U); }
        break;
    case CONFIG_RUNTIME_MARK_FAST:
        if (elapsed>=2000UL) { config_runtime_phase=CONFIG_RUNTIME_MARK_TAIL; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_MARK_TAIL:
        if (elapsed>=2000UL) {
            config_runtime_pulse_index=0;
            if (!config_runtime_seen) {
                config_runtime_phase=CONFIG_RUNTIME_WL_MISSING;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(0U);
            } else {
                config_runtime_pulse_total=config_runtime_wlength_pulses();
                config_runtime_phase=CONFIG_RUNTIME_WL_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            }
        }
        break;
    case CONFIG_RUNTIME_WL_MISSING:
        if (elapsed>=10000UL) { config_runtime_phase=CONFIG_RUNTIME_SEP_TLEN_FAST; config_runtime_phase_ms=now; fan_controller_set_duty(100U); }
        break;
    case CONFIG_RUNTIME_WL_FAST:
        if (elapsed>=2000UL) { config_runtime_phase=CONFIG_RUNTIME_WL_SLOW; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_WL_SLOW:
        if (elapsed>=2000UL) {
            config_runtime_pulse_index++;
            if (config_runtime_pulse_index>=config_runtime_pulse_total) {
                config_runtime_phase=CONFIG_RUNTIME_SEP_TLEN_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            } else {
                config_runtime_phase=CONFIG_RUNTIME_WL_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            }
        }
        break;
    case CONFIG_RUNTIME_SEP_TLEN_FAST:
        if (elapsed>=4000UL) { config_runtime_phase=CONFIG_RUNTIME_SEP_TLEN_SLOW; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_SEP_TLEN_SLOW:
        if (elapsed>=4000UL) {
            config_runtime_tlen_pulses=config_runtime_tlen_class();
            config_runtime_pulse_total=config_runtime_tlen_pulses;
            config_runtime_pulse_index=0;
            config_runtime_phase=CONFIG_RUNTIME_TLEN_FAST;
            config_runtime_phase_ms=now;
            fan_controller_set_duty(100U);
        }
        break;
    case CONFIG_RUNTIME_TLEN_FAST:
        if (elapsed>=2000UL) { config_runtime_phase=CONFIG_RUNTIME_TLEN_SLOW; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_TLEN_SLOW:
        if (elapsed>=2000UL) {
            config_runtime_pulse_index++;
            if (config_runtime_pulse_index>=config_runtime_pulse_total) {
                config_runtime_phase=CONFIG_RUNTIME_SEP_IN_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            } else {
                config_runtime_phase=CONFIG_RUNTIME_TLEN_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            }
        }
        break;
    case CONFIG_RUNTIME_SEP_IN_FAST:
        if (elapsed>=4000UL) { config_runtime_phase=CONFIG_RUNTIME_SEP_IN_SLOW; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_SEP_IN_SLOW:
        if (elapsed>=4000UL) {
            config_runtime_in_pulses=config_runtime_ep0_in_seen?3U:1U;
            config_runtime_pulse_total=config_runtime_in_pulses;
            config_runtime_pulse_index=0;
            config_runtime_phase=CONFIG_RUNTIME_IN_FAST;
            config_runtime_phase_ms=now;
            fan_controller_set_duty(100U);
        }
        break;
    case CONFIG_RUNTIME_IN_FAST:
        if (elapsed>=2000UL) { config_runtime_phase=CONFIG_RUNTIME_IN_SLOW; config_runtime_phase_ms=now; fan_controller_set_duty(0U); }
        break;
    case CONFIG_RUNTIME_IN_SLOW:
        if (elapsed>=2000UL) {
            config_runtime_pulse_index++;
            if (config_runtime_pulse_index>=config_runtime_pulse_total) {
                bool pass=config_runtime_seen && config_runtime_selected &&
                           config_runtime_t_len>0U && config_runtime_ep0_in_seen;
                config_runtime_phase=CONFIG_RUNTIME_FINAL;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(pass?100U:0U);
            } else {
                config_runtime_phase=CONFIG_RUNTIME_IN_FAST;
                config_runtime_phase_ms=now;
                fan_controller_set_duty(100U);
            }
        }
        break;
    case CONFIG_RUNTIME_FINAL:
        if (elapsed>=5000UL) { config_runtime_phase=CONFIG_RUNTIME_DONE; config_runtime_dfu_due=true; }
        break;
    default:
        break;
    }
}
#endif

static void usb_attach_diag_task(void)
{
    uint32_t now=system_millis();
#if FEATURE_USB_RAM_TRACE_DIAG
    if (usb_ram_trace_collecting) {
        if ((uint32_t)(now-usb_ram_trace_started_ms)>=10000UL) {
            usb_trace_freeze();
            usb_ram_trace_collecting=false;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
    if (binary_post_address_result_started) {
        if ((uint32_t)(now-binary_post_address_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_post_address_result_started=false;
            binary_post_address_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
    if (binary_post_address_get_result_started) {
        if ((uint32_t)(now-binary_post_address_get_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_post_address_get_result_started=false;
            binary_post_address_get_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
    if (binary_post_address_config_result_started) {
        if ((uint32_t)(now-binary_post_address_config_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_post_address_config_result_started=false;
            binary_post_address_config_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
    if (binary_post_address_string_result_started) {
        if ((uint32_t)(now-binary_post_address_string_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_post_address_string_result_started=false;
            binary_post_address_string_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
    if (binary_config_seen_result_started) {
        if ((uint32_t)(now-binary_config_seen_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_config_seen_result_started=false;
            binary_config_seen_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_BINARY_WLENGTH9_DIAG
    if (binary_wlength9_result_started) {
        if ((uint32_t)(now-binary_wlength9_result_ms)>=15000UL) {
            fan_controller_set_duty(100U);
            binary_wlength9_result_started=false;
            binary_wlength9_dfu_due=true;
        }
        return;
    }
#endif
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
    if (config_runtime_output_started) {
        config_runtime_diag_task(now);
        return;
    }
#endif
    if (attach_diag_phase==ATTACH_DIAG_PREPARE && (uint32_t)(now-attach_diag_phase_ms)>=2000UL) {
        usb_enable_clocks();
        usb_gpio_init();
        USBFSD->UDEV_CTRL &= (uint8_t)~USBFS_UD_PORT_EN;
        AFIO->CTLR &= ~(USB_IOEN | UDP_PUE_1K5);
        attach_diag_phase=ATTACH_DIAG_DETACH_WAIT;
        attach_diag_phase_ms=now;
    } else if (attach_diag_phase==ATTACH_DIAG_DETACH_WAIT && (uint32_t)(now-attach_diag_phase_ms)>=2000UL) {
        usb_attach();
        attach_diag_phase=ATTACH_DIAG_ATTACH_WAIT;
        attach_diag_phase_ms=now;
#if FEATURE_USB_BINARY_WLENGTH9_DIAG || FEATURE_USB_BINARY_CONFIG_SEEN_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG || FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
        /* The five-second sampling window is deliberately held at 50%. */
        fan_controller_set_duty(50U);
#endif
    } else if (attach_diag_phase==ATTACH_DIAG_ATTACH_WAIT && (uint32_t)(now-attach_diag_phase_ms)>=5000UL) {
        attach_diag_result_seen=usb_bus_reset_seen;
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_ram_trace_collecting=true;
        usb_ram_trace_started_ms=now;
        fan_controller_set_duty(100U);
        return;
#elif FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
        binary_post_address_string_yes=first_post_address_string_setup_captured &&
            first_post_address_string_bRequest==USB_REQ_GET_DESCRIPTOR &&
            (uint8_t)(first_post_address_string_wValue>>8)==USB_DESC_STRING;
        fan_controller_set_duty(binary_post_address_string_yes?100U:0U);
        binary_post_address_string_result_started=true;
        binary_post_address_string_result_ms=now;
        return;
#elif FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
        binary_post_address_config_yes=first_post_address_config_setup_captured &&
            first_post_address_config_bRequest==USB_REQ_GET_DESCRIPTOR &&
            (uint8_t)(first_post_address_config_wValue>>8)==USB_DESC_CONFIGURATION;
        fan_controller_set_duty(binary_post_address_config_yes?100U:0U);
        binary_post_address_config_result_started=true;
        binary_post_address_config_result_ms=now;
        return;
#elif FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
        binary_post_address_get_yes=first_post_address_setup_captured && (first_post_address_bRequest==USB_REQ_GET_DESCRIPTOR);
        fan_controller_set_duty(binary_post_address_get_yes?100U:0U);
        binary_post_address_get_result_started=true;
        binary_post_address_get_result_ms=now;
        return;
#elif FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
        fan_controller_set_duty(post_address_setup_seen?100U:0U);
        binary_post_address_result_started=true;
        binary_post_address_result_ms=now;
        return;
#elif FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
        binary_config_seen_yes=config_seen_request;
        binary_config_seen_result_started=true;
        binary_config_seen_result_ms=now;
        fan_controller_set_duty(binary_config_seen_yes?100U:0U);
        return;
#elif FEATURE_USB_BINARY_WLENGTH9_DIAG
        binary_wlength9_yes=config_request_seen && (first_config_wLength==9U);
        binary_wlength9_result_started=true;
        binary_wlength9_result_ms=now;
        fan_controller_set_duty(binary_wlength9_yes?100U:0U);
        return;
#elif FEATURE_USB_CONFIG_RUNTIME_DIAG
        config_runtime_diag_begin(now);
        return;
#elif FEATURE_USB_SETUP_DIAG
        attach_diag_setup_seen=usb_setup_seen;
        if (!attach_diag_result_seen) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!attach_diag_setup_seen) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_POST_ADDRESS_FIX_DIAG
        if (!usb_address_applied) {
            attach_diag_pulse_total=0U;
            attach_diag_hold_fast=false;
            attach_diag_abnormal=true;
        } else if (!usb_ep0_rearmed_after_address) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_post_address_setup_seen) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_POST_ADDRESS_DIAG
        if (!usb_address_applied) {
            attach_diag_pulse_total=0U;
            attach_diag_hold_fast=false;
            attach_diag_abnormal=true;
        } else if (!usb_ep0_rearmed_after_address) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_post_address_setup_seen) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_CONFIG_IN_DIAG
        attach_diag_zero_event=false;
        if (!config_in_diag_a_seen) {
            attach_diag_pulse_total=0U;
            attach_diag_hold_fast=false;
            attach_diag_zero_event=true;
        } else if (!config_in_diag_b_selected) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!config_in_diag_c_dma_ready) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else if (!config_in_diag_d_tx_armed) {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=false;
        } else if (!config_in_diag_e_in_token_seen) {
            attach_diag_pulse_total=4U;
            attach_diag_hold_fast=false;
        } else if (!config_in_diag_f_in_complete) {
            attach_diag_pulse_total=5U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=6U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_CONFIG_LENGTH_DIAG
        if (!usb_config_length_request_seen) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_config_length_short_in_complete) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else if (!usb_config_length_long_request_seen) {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=false;
        } else if (!usb_config_length_long_in_complete) {
            attach_diag_pulse_total=4U;
            attach_diag_hold_fast=false;
        } else if (!usb_config_length_set_configuration_seen) {
            attach_diag_pulse_total=5U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=5U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_CONFIG_DIAG
        if (!usb_get_config_descriptor_seen) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_config_descriptor_in_complete) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else if (!usb_set_configuration_seen) {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=false;
        } else if (!usb_configuration_applied) {
            attach_diag_pulse_total=4U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=5U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_SET_ADDRESS_DIAG
        if (!usb_device_descriptor_in_complete) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_set_address_seen) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else if (!usb_address_applied) {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=4U;
            attach_diag_hold_fast=true;
        }
#elif FEATURE_USB_DEVICE_DESC_DIAG
        if (!usb_get_device_descriptor_seen) {
            attach_diag_pulse_total=1U;
            attach_diag_hold_fast=false;
        } else if (!usb_device_descriptor_response_started) {
            attach_diag_pulse_total=2U;
            attach_diag_hold_fast=false;
        } else {
            attach_diag_pulse_total=3U;
            attach_diag_hold_fast=true;
        }
#else
        attach_diag_pulse_total=attach_diag_result_seen?3U:1U;
        attach_diag_hold_fast=attach_diag_result_seen;
#endif
        attach_diag_pulse_index=0;
        attach_diag_phase=attach_diag_abnormal?ATTACH_DIAG_RESULT_HOLD:ATTACH_DIAG_RESULT_SLOW;
        attach_diag_phase_ms=now;
        fan_controller_set_duty(0U);
    } else if (attach_diag_phase==ATTACH_DIAG_RESULT_SLOW &&
               (uint32_t)(now-attach_diag_phase_ms)>=(attach_diag_zero_event?6000UL:2000UL)) {
        attach_diag_phase=ATTACH_DIAG_RESULT_FAST;
        attach_diag_phase_ms=now;
        fan_controller_set_duty(100U);
    } else if (attach_diag_phase==ATTACH_DIAG_RESULT_FAST && (uint32_t)(now-attach_diag_phase_ms)>=2000UL) {
        attach_diag_pulse_index++;
        if (attach_diag_pulse_index>=attach_diag_pulse_total) {
            attach_diag_phase=ATTACH_DIAG_RESULT_HOLD;
            fan_controller_set_duty(attach_diag_hold_fast?100U:0U);
        } else {
            attach_diag_phase=ATTACH_DIAG_RESULT_SLOW;
            attach_diag_phase_ms=now;
            fan_controller_set_duty(0U);
        }
    }
}
#else
void usb_attach_diag_start(void) { }
#endif

#if FEATURE_USB_ENUM_TRACE
static void usb_detach_begin(void)
{
    usb_device_state=USB_DEVICE_STATE_DETACHED;
    usb_enable_clocks();
    usb_gpio_init();
    USBFSD->UDEV_CTRL &= (uint8_t)~USBFS_UD_PORT_EN;
    AFIO->CTLR &= ~(USB_IOEN | UDP_PUE_1K5);
    usb_attach_started_ms=system_millis();
    usb_device_state=USB_DEVICE_STATE_WAIT_ATTACH;
}
#else
static void usb_init_hw(void)
{
    usb_enable_clocks();
#if FEATURE_USB_DELAYED_ATTACH_MS
    {
        uint32_t attach_wait_start=system_millis();
        while ((uint32_t)(system_millis()-attach_wait_start)<FEATURE_USB_DELAYED_ATTACH_MS) { }
    }
#endif
    usb_attach();
}
#endif

static void control_stall(void)
{
    USBFSD->UEP0_CTRL_H=USBFS_UEP_T_TOG|USBFS_UEP_T_RES_STALL|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_STALL;
}

static void control_setup(void)
{
    uint8_t *s=ep0_buf;
    uint8_t request_type=s[0];
    uint16_t value=(uint16_t)(s[2]|((uint16_t)s[3]<<8));
    uint16_t requested=(uint16_t)(s[6]|((uint16_t)s[7]<<8));
    uint16_t available=0;
#if FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG
    bool device_descriptor_request=((s[0]&0x9FU)==USB_REQ_DIR_IN) &&
                                    s[1]==USB_REQ_GET_DESCRIPTOR &&
                                    (uint8_t)(value>>8)==USB_DESC_DEVICE;
    device_descriptor_transfer_active=false;
    configuration_descriptor_transfer_active=false;
#endif
    control_request_code=s[1]; control_ptr=0; control_remaining=0;
    if ((s[0]&0x60U)==USB_REQ_TYPE_STANDARD) {
        if (s[1]==USB_REQ_GET_DESCRIPTOR) {
            switch ((uint8_t)(value>>8)) {
            case USB_DESC_DEVICE:
                usb_get_device_desc_count++; usb_enum_trace_request(TRACE_GET_DEVICE_DESCRIPTOR);
#if FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG
                if (device_descriptor_request) usb_get_device_descriptor_seen=true;
                if (device_descriptor_request) device_descriptor_transfer_active=true;
#endif
                control_ptr=device_descriptor; available=sizeof(device_descriptor); break;
            case USB_DESC_CONFIGURATION:
                usb_get_config_desc_count++; usb_enum_trace_request(TRACE_GET_CONFIGURATION_DESCRIPTOR);
#if FEATURE_USB_CONFIG_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG
                if ((s[0]&0x9FU)==USB_REQ_DIR_IN) {
                    usb_get_config_descriptor_seen=true;
                    configuration_descriptor_transfer_active=true;
                }
#if FEATURE_USB_CONFIG_LENGTH_DIAG
                if ((s[0]&0x9FU)==USB_REQ_DIR_IN) {
                    uint8_t request_index=usb_config_length_request_count++;
                    usb_config_length_request_seen=true;
                    if (request_index==0U) {
                        usb_config_length_first_bmRequestType=s[0];
                        usb_config_length_first_bRequest=s[1];
                        usb_config_length_first_wValue=value;
                        usb_config_length_first_wIndex=(uint16_t)(s[4]|((uint16_t)s[5]<<8));
                        usb_config_length_first_wLength=requested;
                    } else if (request_index==1U) {
                        usb_config_length_second_bmRequestType=s[0];
                        usb_config_length_second_bRequest=s[1];
                        usb_config_length_second_wValue=value;
                        usb_config_length_second_wIndex=(uint16_t)(s[4]|((uint16_t)s[5]<<8));
                        usb_config_length_second_wLength=requested;
                    }
                    configuration_request_wLength=requested;
                    if (requested>9U) usb_config_length_long_request_seen=true;
                }
#endif
#endif
                control_ptr=configuration_descriptor; available=sizeof(configuration_descriptor);
#if FEATURE_USB_CONFIG_IN_DIAG
                if (config_in_diag_active && control_ptr != 0 && available != 0U) {
                    config_in_diag_b_selected=true;
                    config_in_diag_descriptor_ptr=(uint32_t)control_ptr;
                    config_in_diag_descriptor_length=available;
                }
#endif
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
                if (config_runtime_active) {
                    config_runtime_selected=true;
                    config_runtime_available=sizeof(configuration_descriptor);
                }
#endif
                break;
            case USB_DESC_STRING:
                usb_get_string_desc_count++; usb_enum_trace_request(TRACE_GET_STRING_DESCRIPTOR);
                if ((uint8_t)value==0) { control_ptr=lang_descriptor; available=sizeof(lang_descriptor); }
                else if ((uint8_t)value==1) { available=string_descriptor("WCH"); control_ptr=ep0_buf; }
                else if ((uint8_t)value==2) { available=string_descriptor("CheeseCool USB HID"); control_ptr=ep0_buf; }
                else if ((uint8_t)value==3) { available=string_descriptor("CC-USB-001"); control_ptr=ep0_buf; }
                else { control_stall(); return; }
                break;
            case USB_DESC_HID: control_ptr=&configuration_descriptor[18]; available=9; break;
            case USB_DESC_REPORT: control_ptr=report_descriptor; available=sizeof(report_descriptor); break;
            default: control_stall(); return;
            }
            control_remaining=(requested<available)?requested:available;
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
            if (config_runtime_active && config_runtime_selected) {
                config_runtime_remaining=control_remaining;
            }
#endif
        } else if (s[1]==USB_REQ_SET_ADDRESS) {
            usb_set_address_count++; usb_enum_trace_request(TRACE_SET_ADDRESS);
#if FEATURE_USB_SET_ADDRESS_DIAG
            if ((s[0]&0x9FU)==0x00U) {
                usb_set_address_seen=true;
                usb_set_address_value=(uint8_t)(value&0x7FU);
            }
#endif
            usb_address=(uint8_t)value;
        } else if (s[1]==USB_REQ_SET_CONFIGURATION) {
            usb_set_configuration_count++; usb_enum_trace_request(TRACE_SET_CONFIGURATION);
#if FEATURE_USB_CONFIG_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG
            if ((s[0]&0x9FU)==0x00U) {
                usb_set_configuration_seen=true;
#if FEATURE_USB_CONFIG_LENGTH_DIAG
                usb_config_length_set_configuration_seen=true;
#endif
            }
#endif
            usb_configuration=(uint8_t)value; configured=(usb_configuration!=0); usb_enum_trace_config_complete=true; endpoint_init();
#if FEATURE_USB_CONFIG_DIAG
            usb_configuration_applied=(usb_configuration!=0);
#endif
        } else if (s[1]==USB_REQ_GET_CONFIGURATION) {
            ep0_buf[0]=usb_configuration; control_ptr=ep0_buf; control_remaining=(requested<1)?requested:1;
        } else if (s[1]==USB_REQ_GET_STATUS) {
            ep0_buf[0]=0; ep0_buf[1]=0; control_ptr=ep0_buf; control_remaining=(requested<2)?requested:2;
        } else if (s[1]==USB_REQ_GET_INTERFACE) {
            ep0_buf[0]=0; control_ptr=ep0_buf; control_remaining=(requested<1)?requested:1;
        } else if (s[1]!=USB_REQ_SET_INTERFACE && s[1]!=USB_REQ_CLEAR_FEATURE) { control_stall(); return; }
    } else if ((s[0]&0x60U)==USB_REQ_TYPE_CLASS) {
        if (s[1]==USB_HID_SET_IDLE) idle_rate=(uint8_t)(value>>8);
        else if (s[1]==USB_HID_SET_PROTOCOL) protocol_mode=(uint8_t)value;
        else if (s[1]==USB_HID_GET_IDLE) { ep0_buf[0]=idle_rate; control_ptr=ep0_buf; control_remaining=1; }
        else if (s[1]==USB_HID_GET_PROTOCOL) { ep0_buf[0]=protocol_mode; control_ptr=ep0_buf; control_remaining=1; }
        else if (s[1]==USB_HID_GET_REPORT) { memset(ep0_buf,0,sizeof(ep0_buf)); control_ptr=ep0_buf; control_remaining=(requested<USB_REPORT_SIZE)?requested:USB_REPORT_SIZE; }
        else if (s[1]!=0x09U) { control_stall(); return; }
    } else { control_stall(); return; }
    if (request_type&USB_REQ_DIR_IN) {
        uint16_t n=(control_remaining>USB_EP0_SIZE)?USB_EP0_SIZE:control_remaining;
        if (control_ptr && n) memcpy(ep0_buf,control_ptr,n);
#if FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG
        if (device_descriptor_request && n) usb_device_descriptor_response_started=true;
#endif
        if (control_ptr) control_ptr+=n;
        control_remaining-=n;
        USBFSD->UEP0_TX_LEN=n;
        USBFSD->UEP0_CTRL_H=USBFS_UEP_T_TOG|USBFS_UEP_T_RES_ACK|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_NAK;
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
        if (config_runtime_active && config_runtime_selected) {
            config_runtime_t_len=USBFSD->UEP0_TX_LEN;
            config_runtime_ctrl_h=USBFSD->UEP0_CTRL_H;
        }
#endif
    } else {
        USBFSD->UEP0_TX_LEN=0; USBFSD->UEP0_CTRL_H=USBFS_UEP_T_TOG|USBFS_UEP_T_RES_ACK|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_NAK;
    }
}

void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void)
{
    uint8_t flags=USBFSD->INT_FG, status=USBFSD->INT_ST;
#if FEATURE_USB_RAM_TRACE_DIAG
    usb_trace_log(USB_TRACE_IRQ_ENTRY);
#endif
    if (flags&USBFS_UIF_TRANSFER) {
        uint8_t token=status&USBFS_UIS_TOKEN_MASK, ep=status&USBFS_UIS_ENDP_MASK;
#if FEATURE_USB_RAM_TRACE_DIAG
        if (token==USBFS_UIS_TOKEN_SETUP && ep==0) {
            usb_trace_log_setup(USB_TRACE_TRANSFER_BEFORE_HANDLE,ep0_buf);
        } else {
            usb_trace_log(USB_TRACE_TRANSFER_BEFORE_HANDLE);
        }
#endif
        if (token==USBFS_UIS_TOKEN_SETUP && ep==0) {
#if FEATURE_USB_SETUP_DIAG
            usb_setup_seen=true;
#endif
#if FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG
            if (usb_address_applied) {
                usb_post_address_setup_seen=true;
                post_address_bmRequestType=ep0_buf[0];
                post_address_bRequest=ep0_buf[1];
                post_address_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                post_address_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                post_address_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
            }
#endif
#if FEATURE_USB_CONFIG_IN_DIAG
            if (!config_in_diag_first_seen &&
                ep0_buf[1]==USB_REQ_GET_DESCRIPTOR &&
                (ep0_buf[0]&0x9FU)==USB_REQ_DIR_IN &&
                (uint8_t)((uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8))>>8)==USB_DESC_CONFIGURATION) {
                config_in_diag_first_seen=true;
                config_in_diag_a_seen=true;
                config_in_diag_active=true;
                config_in_diag_first_bmRequestType=ep0_buf[0];
                config_in_diag_first_bRequest=ep0_buf[1];
                config_in_diag_first_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                config_in_diag_first_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                config_in_diag_first_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
            }
#endif
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
            if (!config_runtime_seen &&
                ep0_buf[1]==USB_REQ_GET_DESCRIPTOR &&
                (ep0_buf[0]&0x9FU)==USB_REQ_DIR_IN &&
                (uint8_t)((uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8))>>8)==USB_DESC_CONFIGURATION) {
                config_runtime_seen=true;
                config_runtime_active=true;
                config_runtime_bmRequestType=ep0_buf[0];
                config_runtime_bRequest=ep0_buf[1];
                config_runtime_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                config_runtime_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                config_runtime_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
            }
#endif
#if FEATURE_USB_BINARY_WLENGTH9_DIAG
            if (!config_request_seen &&
                ep0_buf[1]==USB_REQ_GET_DESCRIPTOR &&
                (ep0_buf[0]&0x9FU)==USB_REQ_DIR_IN &&
                (uint8_t)((uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8))>>8)==USB_DESC_CONFIGURATION) {
                config_request_seen=true;
                first_config_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
            }
#endif
#if FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
            if (!config_seen_request &&
                ep0_buf[1]==USB_REQ_GET_DESCRIPTOR &&
                (ep0_buf[0]&0x9FU)==USB_REQ_DIR_IN &&
                (uint8_t)((uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8))>>8)==USB_DESC_CONFIGURATION) {
                config_seen_request=true;
            }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
            if (address_applied) {
                post_address_setup_seen=true;
            }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
            if (address_applied && !first_post_address_setup_captured) {
                first_post_address_bmRequestType=ep0_buf[0];
                first_post_address_bRequest=ep0_buf[1];
                first_post_address_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                first_post_address_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                first_post_address_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
                first_post_address_setup_captured=true;
            }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
            if (address_applied_config_type && !first_post_address_config_setup_captured) {
                first_post_address_config_bmRequestType=ep0_buf[0];
                first_post_address_config_bRequest=ep0_buf[1];
                first_post_address_config_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                first_post_address_config_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                first_post_address_config_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
                first_post_address_config_setup_captured=true;
            }
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
            if (address_applied_string_type && !first_post_address_string_setup_captured) {
                first_post_address_string_bmRequestType=ep0_buf[0];
                first_post_address_string_bRequest=ep0_buf[1];
                first_post_address_string_wValue=(uint16_t)(ep0_buf[2]|((uint16_t)ep0_buf[3]<<8));
                first_post_address_string_wIndex=(uint16_t)(ep0_buf[4]|((uint16_t)ep0_buf[5]<<8));
                first_post_address_string_wLength=(uint16_t)(ep0_buf[6]|((uint16_t)ep0_buf[7]<<8));
                first_post_address_string_setup_captured=true;
            }
#endif
            usb_setup_count++; usb_enum_trace_request(TRACE_SETUP_RECEIVED);
            USBFSD->UEP0_CTRL_H=USBFS_UEP_T_TOG|USBFS_UEP_T_RES_NAK|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_NAK; control_setup();
        } else if (token==USBFS_UIS_TOKEN_IN) {
            if (ep==0) {
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
                if (config_runtime_active && !config_runtime_ep0_in_seen) {
                    config_runtime_ep0_in_seen=true;
                    config_runtime_int_st=USBFSD->INT_ST;
                    config_runtime_t_len_at_in=USBFSD->UEP0_TX_LEN;
                    config_runtime_ctrl_h_at_in=USBFSD->UEP0_CTRL_H;
                }
#endif
#if FEATURE_USB_CONFIG_IN_DIAG
                if (config_in_diag_active) config_in_diag_e_in_token_seen=true;
#endif
                if (control_remaining) {
                    uint16_t n=(control_remaining>USB_EP0_SIZE)?USB_EP0_SIZE:control_remaining;
                    memcpy(ep0_buf,control_ptr,n);
                    control_ptr+=n;
                    control_remaining-=n;
                    USBFSD->UEP0_TX_LEN=n;
#if FEATURE_USB_CONFIG_IN_DIAG
                    if (config_in_diag_active && n>0U) {
                        config_in_diag_c_dma_ready=true;
                        config_in_diag_send_length=n;
                        config_in_diag_ep0_data=(uint32_t)ep0_buf;
                        config_in_diag_ep0_dma=USBFSD->UEP0_DMA;
                        config_in_diag_t_len=USBFSD->UEP0_TX_LEN;
                    }
#endif
                    USBFSD->UEP0_CTRL_H^=USBFS_UEP_T_TOG;
#if FEATURE_USB_CONFIG_IN_DIAG
                    if (config_in_diag_active && n>0U &&
                        (USBFSD->UEP0_CTRL_H&USBFS_UEP_T_RES_MASK)==USBFS_UEP_T_RES_ACK) {
                        config_in_diag_d_tx_armed=true;
                        config_in_diag_ctrl_h=USBFSD->UEP0_CTRL_H;
                    }
                    if (config_in_diag_active && control_remaining==0U) {
                        config_in_diag_f_in_complete=true;
                        config_in_diag_active=false;
                    }
#endif
#if FEATURE_USB_CONFIG_LENGTH_DIAG
                    if (configuration_descriptor_transfer_active && control_remaining==0U) {
                        if (configuration_request_wLength<=9U) usb_config_length_short_in_complete=true;
                        else usb_config_length_long_in_complete=true;
                        configuration_descriptor_transfer_active=false;
                    }
#endif
                }
                else {
#if FEATURE_USB_CONFIG_DIAG
                    if (configuration_descriptor_transfer_active) {
                        usb_config_descriptor_in_complete=true;
                        configuration_descriptor_transfer_active=false;
                    }
#endif
#if FEATURE_USB_SET_ADDRESS_DIAG
                    if (device_descriptor_transfer_active) {
                        usb_device_descriptor_in_complete=true;
                        device_descriptor_transfer_active=false;
                    }
#endif
                    USBFSD->UEP0_TX_LEN=0;
                    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_TOG|USBFS_UEP_T_RES_ACK;
                    /* Arm the status OUT stage after the final control IN packet. */
                    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_ACK;
                    if (control_request_code==USB_REQ_SET_ADDRESS) {
                        USBFSD->DEV_ADDR=(USBFSD->DEV_ADDR&USBFS_UDA_GP_BIT)|usb_address;
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
                        address_applied=true;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
                        address_applied=true;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
                        address_applied_config_type=true;
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
                        address_applied_string_type=true;
#endif
                        /* WCH EVT behavior: re-arm EP0 RX after the
                         * SET_ADDRESS status IN has completed. */
                        USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_ACK;
#if FEATURE_USB_SET_ADDRESS_DIAG
                        usb_address_applied=true;
                        usb_address_applied_value=usb_address&0x7FU;
#endif
#if FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG
                        usb_ep0_ctrl_after_address=USBFSD->UEP0_CTRL_H;
                        usb_ep0_rearmed_after_address=
                            ((usb_ep0_ctrl_after_address&USBFS_UEP_R_RES_MASK)==USBFS_UEP_R_RES_ACK);
#endif
                    }
                }
            } else if (ep==1) { tx_busy=false; tx_done=true; USBFSD->UEP1_CTRL_H=(USBFSD->UEP1_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_RES_NAK; USBFSD->UEP1_CTRL_H^=USBFS_UEP_T_TOG; }
        } else if (token==USBFS_UIS_TOKEN_OUT) {
            if (ep==0) USBFSD->UEP0_TX_LEN=0;
            else if (ep==2 && !out_pending) { memcpy(pending_out,ep2_out_buf,USB_REPORT_SIZE); out_pending=true; USBFSD->UEP2_CTRL_H=(USBFSD->UEP2_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_RES_NAK; }
        }
        USBFSD->INT_FG=USBFS_UIF_TRANSFER;
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_trace_log(USB_TRACE_TRANSFER_AFTER_CLEAR);
#endif
    } else if (flags&USBFS_UIF_BUS_RST) {
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_trace_log(USB_TRACE_BUS_RST_BEFORE_HANDLE);
#endif
#if FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
        if (address_applied) post_address_bus_reset_seen=true;
#endif
#if FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG
        usb_bus_reset_seen=true;
#endif
        usb_bus_reset_count++; usb_enum_trace_request(TRACE_BUS_RESET); configured=false; usb_configuration=0; usb_address=0; endpoint_init(); USBFSD->DEV_ADDR=0; USBFSD->INT_FG=USBFS_UIF_BUS_RST;
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_trace_log(USB_TRACE_BUS_RST_AFTER_CLEAR);
#endif
    }
    else if (flags&USBFS_UIF_SUSPEND) {
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_trace_log(USB_TRACE_SUSPEND_BEFORE_HANDLE);
#endif
        USBFSD->INT_FG=USBFS_UIF_SUSPEND;
#if FEATURE_USB_RAM_TRACE_DIAG
        usb_trace_log(USB_TRACE_SUSPEND_AFTER_CLEAR);
#endif
    }
    else USBFSD->INT_FG=flags;
#if FEATURE_USB_RAM_TRACE_DIAG
    usb_trace_log(USB_TRACE_IRQ_EXIT);
#endif
}

void usb_device_init(void)
{
#if FEATURE_USB_DEVICE
    configured=false; out_pending=false; tx_busy=false; tx_done=false; dfu_reset_waiting=false; dfu_reset_due_ms=0U; protocol_mode=1; idle_rate=0;
    diag_start_ms=system_millis(); usb_protocol_init();
#if FEATURE_USB_ENUM_TRACE
    usb_detach_begin();
#elif FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG
    /* Attach diagnostic performs delayed detach/attach from the main loop. */
#else
    usb_init_hw();
#endif
#endif
}

void usb_enum_trace_start(void)
{
#if FEATURE_USB_ENUM_TRACE
    usb_enum_trace_stage=TRACE_BOOT;
    final_trace_stage=0;
    usb_enum_trace_pending_stage=TRACE_BOOT;
    usb_enum_trace_frozen=false;
    usb_enum_trace_start_ms=system_millis();
    usb_enum_trace_phase_ms=usb_enum_trace_start_ms;
    usb_enum_trace_pulse_index=0;
    usb_enum_trace_phase=TRACE_PHASE_COLLECT;
    fan_controller_enable();
    fan_controller_set_duty(10U);
#endif
}

void usb_enum_trace_task(void)
{
#if FEATURE_USB_ENUM_TRACE
    uint32_t now=system_millis();
    if (!usb_enum_trace_frozen && (uint32_t)(now-usb_enum_trace_start_ms)>=5000UL) {
        final_trace_stage=usb_enum_trace_stage;
        if (final_trace_stage<TRACE_BOOT || final_trace_stage>TRACE_HID_READY) final_trace_stage=TRACE_BOOT;
        usb_enum_trace_frozen=true;
        usb_enum_trace_phase=TRACE_PHASE_MARKER;
        usb_enum_trace_phase_ms=now;
        fan_controller_set_duty(100U);
    } else if (!usb_enum_trace_frozen && usb_enum_trace_stage==TRACE_SET_CONFIGURATION && usb_enum_trace_config_complete) {
        usb_enum_trace_config_complete=false;
        usb_enum_trace_pending_stage=TRACE_HID_READY;
    } else if (usb_enum_trace_frozen) {
        if (usb_enum_trace_phase==TRACE_PHASE_MARKER && (uint32_t)(now-usb_enum_trace_phase_ms)>=3000UL) {
            usb_enum_trace_phase=TRACE_PHASE_SLOW;
            usb_enum_trace_phase_ms=now;
            usb_enum_trace_pulse_index=0;
            fan_controller_set_duty(0U);
        } else if (usb_enum_trace_phase==TRACE_PHASE_SLOW && (uint32_t)(now-usb_enum_trace_phase_ms)>=2000UL) {
            usb_enum_trace_phase=TRACE_PHASE_FAST;
            usb_enum_trace_phase_ms=now;
            fan_controller_set_duty(100U);
        } else if (usb_enum_trace_phase==TRACE_PHASE_FAST && (uint32_t)(now-usb_enum_trace_phase_ms)>=2000UL) {
            usb_enum_trace_pulse_index++;
            if (usb_enum_trace_pulse_index>=final_trace_stage) {
                usb_enum_trace_phase=TRACE_PHASE_HOLD;
                fan_controller_set_duty(100U);
            } else {
                usb_enum_trace_phase=TRACE_PHASE_SLOW;
                usb_enum_trace_phase_ms=now;
                fan_controller_set_duty(0U);
            }
        }
    }
#endif
}

void usb_device_task(void)
{
#if FEATURE_USB_DEVICE
#if FEATURE_USB_ATTACH_DIAG || FEATURE_USB_SETUP_DIAG || FEATURE_USB_DEVICE_DESC_DIAG || FEATURE_USB_SET_ADDRESS_DIAG || FEATURE_USB_CONFIG_DIAG || FEATURE_USB_POST_ADDRESS_DIAG || FEATURE_USB_POST_ADDRESS_FIX_DIAG || FEATURE_USB_CONFIG_LENGTH_DIAG || FEATURE_USB_CONFIG_IN_DIAG || FEATURE_USB_CONFIG_RUNTIME_DIAG
    usb_attach_diag_task();
#endif
#if FEATURE_USB_ENUM_TRACE
    if (usb_device_state==USB_DEVICE_STATE_WAIT_ATTACH &&
        (uint32_t)(system_millis()-usb_attach_started_ms)>=20UL) {
        usb_attach();
        usb_device_state=USB_DEVICE_STATE_ATTACHED;
    }
#endif
    uint8_t request[USB_REPORT_SIZE], response[USB_REPORT_SIZE];
    if (usb_device_receive_report(request)) { usb_protocol_process(request,response); usb_device_send_report(response); }
    if (usb_protocol_dfu_pending()) {
        if (!dfu_reset_waiting && usb_device_tx_complete()) {
            dfu_reset_waiting=true;
            dfu_reset_due_ms=system_millis()+20UL;
        }
        if (dfu_reset_waiting && (uint32_t)(system_millis()-dfu_reset_due_ms)<0x80000000UL) system_request_dfu();
    }
#if FEATURE_USB_CONFIG_RUNTIME_DIAG
    if (config_runtime_dfu_due) {
        config_runtime_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_WLENGTH9_DIAG
    if (binary_wlength9_dfu_due) {
        binary_wlength9_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_CONFIG_SEEN_DIAG
    if (binary_config_seen_dfu_due) {
        binary_config_seen_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_POST_ADDRESS_SETUP_DIAG
    if (binary_post_address_dfu_due) {
        binary_post_address_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_POST_ADDRESS_GET_DESCRIPTOR_DIAG
    if (binary_post_address_get_dfu_due) {
        binary_post_address_get_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_POST_ADDRESS_CONFIG_TYPE_DIAG
    if (binary_post_address_config_dfu_due) {
        binary_post_address_config_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_BINARY_POST_ADDRESS_STRING_TYPE_DIAG
    if (binary_post_address_string_dfu_due) {
        binary_post_address_string_dfu_due=false;
        system_request_dfu();
    }
#elif FEATURE_USB_RAM_TRACE_DIAG
    /* Keep the frozen RAM trace available; no automatic DFU is requested. */
#elif FEATURE_USB_DIAG
    if (!usb_protocol_dfu_pending() && (uint32_t)(system_millis()-diag_start_ms)>=USB_DIAG_TIMEOUT_MS) system_request_dfu();
#endif
#endif
}

bool usb_device_send_report(const uint8_t report[USB_REPORT_SIZE])
{
#if FEATURE_USB_DEVICE
    if (!configured || tx_busy) return false;
    memcpy(tx_report,report,USB_REPORT_SIZE); memcpy(ep1_in_buf,tx_report,USB_REPORT_SIZE);
    USBFSD->UEP1_TX_LEN=USB_REPORT_SIZE; USBFSD->UEP1_CTRL_H=(USBFSD->UEP1_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_RES_ACK; tx_busy=true; tx_done=false; return true;
#else
    (void)report; return false;
#endif
}

bool usb_device_receive_report(uint8_t report[USB_REPORT_SIZE])
{
#if FEATURE_USB_DEVICE
    if (!out_pending) return false;
    memcpy(report,pending_out,USB_REPORT_SIZE); out_pending=false; USBFSD->UEP2_CTRL_H=(USBFSD->UEP2_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_RES_ACK; return true;
#else
    (void)report; return false;
#endif
}

bool usb_device_is_configured(void) { return configured; }
bool usb_device_tx_complete(void) { bool done=tx_done; tx_done=false; return done; }
