#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "usb_protocol.h"
#include "fan_controller.h"
#include "failsafe.h"
#include "power_monitor.h"
#include "system_status.h"
#include "usb_device.h"

static device_status_t status;
static unsigned host_activity_count;
static uint8_t duty;
static fan_mode_t mode;

const device_status_t *system_status_get(void) { return &status; }
uint32_t system_millis(void) { return 0U; }
uint8_t fan_controller_get_duty(void) { return duty; }
uint32_t fan_controller_get_rpm(void) { return 0U; }
void fan_controller_set_duty(uint8_t value) { duty = value; }
void fan_controller_enable(void) { }
void fan_controller_disable(void) { }
bool fan_controller_is_enabled(void) { return false; }
void fan_controller_set_mode(fan_mode_t value) { mode = value; }
fan_mode_t fan_controller_get_mode(void) { return mode; }
bool fan_controller_set_curve(const fan_curve_point_t *points, uint8_t count)
{
    (void)points;
    (void)count;
    return true;
}
bool failsafe_is_active(void) { return false; }
void failsafe_host_activity(void) { host_activity_count++; }
bool power_monitor_has_fault(void) { return false; }
bool usb_device_is_configured(void) { return true; }

static uint8_t checksum(const uint8_t report[USB_REPORT_SIZE])
{
    uint8_t value = 0U;
    for (uint8_t index = 0U; index < USB_REPORT_SIZE - 1U; index++) {
        value ^= report[index];
    }
    return value;
}

static void test_reserved_command(uint8_t command)
{
    uint8_t request[USB_REPORT_SIZE] = {0};
    uint8_t response[USB_REPORT_SIZE] = {0};

    request[0] = PROTOCOL_VERSION;
    request[1] = command;
    request[2] = 0x5AU;
    request[USB_REPORT_SIZE - 1U] = checksum(request);

    usb_protocol_process(request, response);

    assert(response[0] == PROTOCOL_VERSION);
    assert(response[1] == command);
    assert(response[2] == 0x5AU);
    assert(response[3] == 1U);
    assert(response[4] == USB_STATUS_BAD_COMMAND);
    assert(response[USB_REPORT_SIZE - 1U] == checksum(response));
    assert(host_activity_count == 0U);
    assert(duty == 0U);
    assert(mode == FAN_MODE_HOST_CONTROLLED);
}

int main(void)
{
    usb_protocol_init();
    test_reserved_command(CMD_RESERVED_08);
    test_reserved_command(CMD_RESERVED_0D);
    return 0;
}
