#include <stdint.h>
#include "ch32x035.h"
#include "ch32x035_usb.h"
#include "ch32x035_rcc.h"
#include "ch32x035_gpio.h"
#include "ram_usb_dma.h"
#include "ram_usb_variant.h"
#include "ram_usb_sections.h"

#define EP0_SIZE 64u
#define USB_IOEN 0x80u
#define USB_PHY_V33 0x40u
#define UDP_PUE_MASK 0x0Cu
#define UDP_PUE_1K5 0x0Cu
#define UDM_PUE_MASK 0x03u
#define USB_VID 0x1A86u
#define USB_PID 0x8035u
#define RAM_USB_TRACE_CAPACITY 64u

enum {
    TRACE_BOOT = 1,
    TRACE_USB_INIT_BEGIN,
    TRACE_CLOCK_READY,
    TRACE_ENDPOINT_INIT,
    TRACE_FLAGS_CLEARED,
    TRACE_PULLUP_ENABLED,
    TRACE_BUS_RST,
    TRACE_SET_ADDRESS_SETUP,
    TRACE_SET_ADDRESS_STATUS,
    TRACE_SET_ADDRESS_COMMIT,
    TRACE_GET_DESCRIPTOR_SETUP,
    TRACE_GET_DESCRIPTOR_TX,
    TRACE_EP0_IN,
    TRACE_EP0_OUT,
    TRACE_OTHER_TRANSFER
};

typedef struct {
    uint32_t sequence;
    uint8_t event;
    uint8_t int_fg;
    uint8_t int_st;
    uint8_t mis_st;
    uint8_t dev_addr;
    uint8_t ep0_ctrl_h;
    uint16_t ep0_tx_len;
    uint16_t ep0_dma;
    uint8_t request_type;
    uint8_t request_code;
    uint16_t request_value;
    uint16_t request_length;
    uint16_t control_remaining;
    uint8_t setup[8];
    uint8_t tx_data[16];
} ram_usb_trace_entry_t;

RAM_USB_DMA_OBJECT static uint8_t ep0_buf[EP0_SIZE];

RAM_USB_TRACE_OBJECT static volatile ram_usb_trace_entry_t trace_buffer[RAM_USB_TRACE_CAPACITY];
extern volatile uint32_t ram_usb_trace_count;
extern volatile uint32_t ram_usb_trace_overflow;
static volatile uint8_t usb_address, usb_configuration;
static uint8_t request_type, request_code;
static uint16_t request_length, control_remaining;
static const uint8_t *control_pointer;

static void trace_log(uint8_t event, uint8_t capture_setup, uint8_t capture_tx)
{
    uint32_t index = ram_usb_trace_count;
    if (index >= RAM_USB_TRACE_CAPACITY) { ram_usb_trace_overflow = 1; return; }
    ram_usb_trace_entry_t *entry = (ram_usb_trace_entry_t *)&trace_buffer[index];
    entry->sequence = index;
    entry->event = event;
    entry->int_fg = USBFSD->INT_FG;
    entry->int_st = USBFSD->INT_ST;
    entry->mis_st = USBFSD->MIS_ST;
    entry->dev_addr = USBFSD->DEV_ADDR;
    entry->ep0_ctrl_h = USBFSD->UEP0_CTRL_H;
    entry->ep0_tx_len = USBFSD->UEP0_TX_LEN;
    entry->ep0_dma = USBFSD->UEP0_DMA;
    entry->request_type = request_type;
    entry->request_code = request_code;
    entry->request_value = (uint16_t)ep0_buf[2] | ((uint16_t)ep0_buf[3] << 8);
    entry->request_length = request_length;
    entry->control_remaining = control_remaining;
    if (capture_setup) {
        for (uint8_t i = 0; i < 8; ++i) entry->setup[i] = ep0_buf[i];
    }
    if (capture_tx) {
        uint16_t n = entry->ep0_tx_len < sizeof(entry->tx_data) ? entry->ep0_tx_len : sizeof(entry->tx_data);
        for (uint8_t i = 0; i < 16; ++i) entry->tx_data[i] = i < n ? ep0_buf[i] : 0;
    }
    ram_usb_trace_count = index + 1;
}

RAM_USB_DESCRIPTOR_OBJECT static const uint8_t device_descriptor[] = {
    0x12,0x01,0x00,0x02,0x00,0x00,0x00,EP0_SIZE,
    (uint8_t)USB_VID,(uint8_t)(USB_VID >> 8),
    (uint8_t)USB_PID,(uint8_t)(USB_PID >> 8), 0x00,0x01,0x01,0x02,0x03,0x01
};
RAM_USB_DESCRIPTOR_OBJECT static const uint8_t configuration_descriptor[] = {
    0x09,0x02,0x1B,0x00,0x01,0x01,0x00,0x80,0x32,
    0x09,0x04,0x00,0x00,0x00,0xFE,0x01,0x02,0x04,
    0x09,0x21,0x07,0xFF,0x00,0x40,0x00,0x10,0x01
};
RAM_USB_DESCRIPTOR_OBJECT static const uint8_t language_descriptor[] = {0x04,0x03,0x09,0x04};

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t length)
{ while (length--) *dst++ = *src++; }

static void endpoint_init(void)
{
    USBFSD->UEP4_1_MOD=0; USBFSD->UEP2_3_MOD=0;
    USBFSD->UEP0_DMA=USB_DMA_ADDR(ep0_buf);
    USBFSD->UEP0_CTRL_H=USBFS_UEP_R_RES_ACK|USBFS_UEP_T_RES_NAK;
}

static void arm_status_in(void)
{
    USBFSD->UEP0_TX_LEN=0;
    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_TOG|USBFS_UEP_T_RES_ACK;
    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_ACK;
}

static void arm_first_in(void)
{
    uint16_t n=control_remaining>EP0_SIZE?EP0_SIZE:control_remaining;
    copy_bytes(ep0_buf,control_pointer,n); control_pointer+=n; control_remaining-=n;
    USBFSD->UEP0_TX_LEN=n;
    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_RES_ACK;
}

static void arm_next_in(void)
{
    uint16_t n=control_remaining>EP0_SIZE?EP0_SIZE:control_remaining;
    copy_bytes(ep0_buf,control_pointer,n); control_pointer+=n; control_remaining-=n;
    USBFSD->UEP0_TX_LEN=n;
    USBFSD->UEP0_CTRL_H^=USBFS_UEP_T_TOG;
    USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_T_RES_MASK)|USBFS_UEP_T_RES_ACK;
}

static void handle_setup(void)
{
    uint16_t value=(uint16_t)ep0_buf[2]|((uint16_t)ep0_buf[3]<<8);
    const uint8_t *descriptor=0; uint16_t descriptor_length=0;
    request_type=ep0_buf[0]; request_code=ep0_buf[1];
    request_length=(uint16_t)ep0_buf[6]|((uint16_t)ep0_buf[7]<<8);
    control_pointer=0; control_remaining=0;
    if ((request_type&USB_REQ_TYP_MASK)==USB_REQ_TYP_STANDARD) {
        if (request_code==USB_GET_DESCRIPTOR) {
            switch ((uint8_t)(value>>8)) {
            case 1: descriptor=device_descriptor; descriptor_length=sizeof(device_descriptor); break;
            case 2: descriptor=configuration_descriptor; descriptor_length=sizeof(configuration_descriptor); break;
            case 3: descriptor=language_descriptor; descriptor_length=sizeof(language_descriptor); break;
            default: break;
            }
            if (descriptor) { if (request_length>descriptor_length) request_length=descriptor_length;
                control_pointer=descriptor; control_remaining=request_length; }
        } else if (request_code==USB_SET_ADDRESS) usb_address=(uint8_t)value;
        else if (request_code==USB_SET_CONFIGURATION) usb_configuration=(uint8_t)value;
        else if (request_code==USB_GET_CONFIGURATION) {
            ep0_buf[0]=usb_configuration; request_length=request_length?1u:0u;
            control_pointer=ep0_buf; control_remaining=request_length;
        } else request_length=0;
    } else request_length=0;
    if (request_type&USB_REQ_TYP_IN) { if (control_remaining) arm_first_in(); else arm_status_in(); }
    else arm_status_in();
}

static void handle_transfer(uint8_t status)
{
    uint8_t token=status&USBFS_UIS_TOKEN_MASK, endpoint=status&USBFS_UIS_ENDP_MASK;
    if (endpoint!=0) { trace_log(TRACE_OTHER_TRANSFER, 0, 0); return; }
    if (token==USBFS_UIS_TOKEN_SETUP) {
        USBFSD->UEP0_CTRL_H=USBFS_UEP_T_TOG|USBFS_UEP_T_RES_NAK|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_NAK;
        uint8_t code = ep0_buf[1];
        trace_log(code == USB_SET_ADDRESS ? TRACE_SET_ADDRESS_SETUP :
                  (code == USB_GET_DESCRIPTOR ? TRACE_GET_DESCRIPTOR_SETUP : TRACE_OTHER_TRANSFER), 1, 0);
        handle_setup();
        if (request_type & USB_REQ_TYP_IN)
            trace_log(request_code == USB_GET_DESCRIPTOR ? TRACE_GET_DESCRIPTOR_TX : TRACE_SET_ADDRESS_STATUS, 0, 1);
    } else if (token==USBFS_UIS_TOKEN_IN) {
        if (control_remaining) arm_next_in();
        else { USBFSD->UEP0_TX_LEN=0; arm_status_in();
                if (request_code==USB_SET_ADDRESS) {
                USBFSD->DEV_ADDR=(USBFSD->DEV_ADDR&USBFS_UDA_GP_BIT)|usb_address;
                USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_ACK;
                trace_log(TRACE_SET_ADDRESS_COMMIT, 0, 0);
            }
            }
        trace_log(TRACE_EP0_IN, 0, 0);
    } else if (token==USBFS_UIS_TOKEN_OUT) {
        USBFSD->UEP0_TX_LEN=0;
        USBFSD->UEP0_CTRL_H=(USBFSD->UEP0_CTRL_H&~USBFS_UEP_R_RES_MASK)|USBFS_UEP_R_TOG|USBFS_UEP_R_RES_ACK;
        trace_log(TRACE_EP0_OUT, 0, 0);
    }
}

static void handle_bus_reset(void)
{ usb_address=0; usb_configuration=0; USBFSD->DEV_ADDR=0; endpoint_init(); USBFSD->INT_FG=USBFS_UIF_BUS_RST; }

void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void)
{
    uint8_t flags=USBFSD->INT_FG, status=USBFSD->INT_ST;
#if USB_IRQ_BUS_RESET_FIRST
    if (flags&USBFS_UIF_BUS_RST) { trace_log(TRACE_BUS_RST, 0, 0); handle_bus_reset(); }
    else if (flags&USBFS_UIF_TRANSFER) { handle_transfer(status); USBFSD->INT_FG=USBFS_UIF_TRANSFER; }
#else
    if (flags&USBFS_UIF_TRANSFER) { handle_transfer(status); USBFSD->INT_FG=USBFS_UIF_TRANSFER; }
    else if (flags&USBFS_UIF_BUS_RST) { trace_log(TRACE_BUS_RST, 0, 0); handle_bus_reset(); }
#endif
    else if (flags&USBFS_UIF_SUSPEND) { USBFSD->INT_FG=USBFS_UIF_SUSPEND; }
    else { USBFSD->INT_FG=flags; }
}

void usb_ram_diag_init(void)
{
    GPIO_InitTypeDef gpio={0};
    uint32_t group;
    ram_usb_trace_count = 0; ram_usb_trace_overflow = 0;
    trace_log(TRACE_BOOT, 0, 0); trace_log(TRACE_USB_INIT_BEGIN, 0, 0);
    /* Do not inherit pending/active interrupts from the program we replaced. */
    for (group=0; group<8; ++group) {
        PFIC->IRER[group]=0xFFFFFFFFu;
        PFIC->IPRR[group]=0xFFFFFFFFu;
    }
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS,ENABLE);
    trace_log(TRACE_CLOCK_READY, 0, 0);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    gpio.GPIO_Pin=GPIO_Pin_16; gpio.GPIO_Mode=GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOC,&gpio);
    gpio.GPIO_Pin=GPIO_Pin_17; gpio.GPIO_Mode=GPIO_Mode_IPU; GPIO_Init(GPIOC,&gpio);
    AFIO->CTLR=(AFIO->CTLR&~(UDP_PUE_MASK|UDM_PUE_MASK))|USB_PHY_V33|UDP_PUE_1K5|USB_IOEN;
    USBFSD->BASE_CTRL=0; endpoint_init(); trace_log(TRACE_ENDPOINT_INIT, 0, 0); USBFSD->DEV_ADDR=0;
    USBFSD->BASE_CTRL=USBFS_UC_DEV_PU_EN|USBFS_UC_INT_BUSY|USBFS_UC_DMA_EN;
    USBFSD->INT_FG=0xFF; trace_log(TRACE_FLAGS_CLEARED, 0, 0); USBFSD->UDEV_CTRL=USBFS_UD_PD_DIS|USBFS_UD_PORT_EN; trace_log(TRACE_PULLUP_ENABLED, 0, 0);
    USBFSD->INT_EN=USBFS_UIE_SUSPEND|USBFS_UIE_BUS_RST|USBFS_UIE_TRANSFER;
    NVIC_EnableIRQ(USBFS_IRQn);
    __enable_irq();
}
