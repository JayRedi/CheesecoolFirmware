#include "usb_protocol.h"
#include "firmware_version.h"
#include "board_config.h"
#include "fan_controller.h"
#include "failsafe.h"
#include "system_status.h"
#include "power_monitor.h"
#include "usb_device.h"
static uint8_t checksum(const uint8_t *d) { uint8_t c=0; for(uint8_t i=0;i<USB_REPORT_SIZE-1U;i++) c^=d[i]; return c; }
static void put32(uint8_t *p,uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static void put_status(uint8_t *payload) {
    const device_status_t *s=system_status_get();
    uint8_t duty=fan_controller_get_duty();
    payload[0]=s->mode;
    payload[1]=duty;
    payload[2]=duty;
    put32(&payload[3],s->fan_rpm);
    payload[7]=usb_device_is_configured()?1U:0U;
    payload[8]=s->failsafe_active?1U:0U;
    payload[9]=s->power_fault?1U:0U;
    put32(&payload[10],s->uptime_ms);
    payload[14]=FIRMWARE_VERSION_MAJOR;
    payload[15]=FIRMWARE_VERSION_MINOR;
    payload[16]=FIRMWARE_VERSION_PATCH;
}
static void set_duty_command(const uint8_t *request, uint8_t *response) {
    if(request[3]!=1U || request[4]>100U) {
        response[4]=USB_STATUS_BAD_PARAMETER;
    } else if(fan_controller_get_mode()!=FAN_MODE_HOST_CONTROLLED || failsafe_is_active() || power_monitor_has_fault()) {
        response[4]=USB_STATUS_NOT_SUPPORTED;
    } else {
        fan_controller_set_duty(request[4]);
    }
}
static void set_curve_command(const uint8_t *request, uint8_t *response) {
    fan_curve_point_t candidate[FAN_CURVE_MAX_POINTS];
    uint8_t count=request[4];
    uint8_t expected;
    if(count==0U || count>FAN_CURVE_MAX_POINTS) { response[4]=USB_STATUS_BAD_PARAMETER; return; }
    expected=(uint8_t)(1U+2U*count);
    if(request[3]!=expected) { response[4]=USB_STATUS_BAD_PARAMETER; return; }
    for(uint8_t i=0U;i<count;i++) {
        candidate[i].temperature_c=request[5U+2U*i];
        candidate[i].duty_percent=request[6U+2U*i];
        if(candidate[i].temperature_c>125U || candidate[i].duty_percent>100U ||
           (i>0U && candidate[i-1U].temperature_c>=candidate[i].temperature_c)) {
            response[4]=USB_STATUS_BAD_PARAMETER;
            return;
        }
    }
    if(!fan_controller_set_curve(candidate,count)) { response[4]=USB_STATUS_BAD_PARAMETER; return; }
    response[3]=expected;
    response[5]=count;
    for(uint8_t i=0U;i<count;i++) { response[6U+2U*i]=candidate[i].temperature_c; response[7U+2U*i]=candidate[i].duty_percent; }
}
static bool command_is_known(usb_command_t command)
{
    switch (command) {
    case CMD_PING:
    case CMD_GET_INFO:
    case CMD_SET_FAN_DUTY:
    case CMD_GET_FAN_STATUS:
    case CMD_FAN_ENABLE:
    case CMD_FAN_DISABLE:
    case CMD_KEEPALIVE:
    case CMD_GET_STATUS:
    case CMD_SET_MODE:
    case CMD_SET_DUTY:
    case CMD_SET_CURVE:
        return true;
    default:
        return false;
    }
}
static void base(const uint8_t *r,uint8_t *s,uint8_t st) { for(uint8_t i=0;i<USB_REPORT_SIZE;i++) s[i]=0; s[0]=PROTOCOL_VERSION; s[1]=r[1]; s[2]=r[2]; s[3]=1; s[4]=st; }
void usb_protocol_init(void) { }
void usb_protocol_process(const uint8_t request[USB_REPORT_SIZE],uint8_t response[USB_REPORT_SIZE]) {
    base(request,response,USB_STATUS_OK);
    if(request[0]!=PROTOCOL_VERSION || request[3]>(USB_REPORT_SIZE-5U) || request[USB_REPORT_SIZE-1U]!=checksum(request)) { base(request,response,USB_STATUS_BAD_PACKET); response[USB_REPORT_SIZE-1U]=checksum(response); return; }
    bool known = command_is_known((usb_command_t)request[1]);
    switch((usb_command_t)request[1]) {
    case CMD_PING: response[3]=5; response[5]=0x50; response[6]=0x4F; response[7]=0x4E; response[8]=0x47; break;
    case CMD_GET_INFO: response[3]=7; response[5]=FIRMWARE_VERSION_MAJOR; response[6]=FIRMWARE_VERSION_MINOR; response[7]=FIRMWARE_VERSION_PATCH; response[8]=PROTOCOL_VERSION; response[9]=USB_BOOTLOADER_SUPPORTED; response[10]=(uint8_t)system_status_get()->state; break;
    case CMD_SET_FAN_DUTY: set_duty_command(request,response); break;
    case CMD_GET_FAN_STATUS: { const device_status_t *s=system_status_get(); response[3]=9; response[5]=s->fan_duty; response[6]=s->fan_enabled; response[7]=s->failsafe_active; response[8]=s->power_fault; put32(&response[9],s->fan_rpm); } break;
    case CMD_GET_STATUS: put_status(&response[5]); response[3]=17; break;
    case CMD_SET_MODE:
        if(request[3]!=1U || request[4]>(uint8_t)FAN_MODE_MAX) response[4]=USB_STATUS_BAD_PARAMETER;
        else if(failsafe_is_active() || power_monitor_has_fault()) response[4]=USB_STATUS_NOT_SUPPORTED;
        else { fan_controller_set_mode((fan_mode_t)request[4]); }
        break;
    case CMD_SET_DUTY: set_duty_command(request,response); break;
    case CMD_SET_CURVE: set_curve_command(request,response); break;
    case CMD_FAN_ENABLE: fan_controller_enable(); break;
    case CMD_FAN_DISABLE: fan_controller_disable(); break;
    case CMD_KEEPALIVE: break;
    case CMD_RESERVED_08:
    case CMD_RESERVED_0D:
        response[4]=USB_STATUS_BAD_COMMAND;
        break;
    default: response[4]=USB_STATUS_BAD_COMMAND; break;
    }
    if (known) failsafe_host_activity();
    response[USB_REPORT_SIZE-1U]=checksum(response);
}
