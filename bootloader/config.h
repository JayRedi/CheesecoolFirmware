#ifndef CHEESECOOL_DFU_CONFIG_H
#define CHEESECOOL_DFU_CONFIG_H

#define BOOT_BASE              0x00000000u
#define BOOT_SIZE              0x2000u
#define APP_BASE               0x00002000u
#define APP_SIZE               (0xF800u - APP_BASE)

#define BOOT_FLAG_ADDR         0x20004FF0u
#define BOOT_MAGIC_DFU         0xB0071DF0u
#define DFU_ENTER_IF_NO_APP    1
#define DFU_BUTTON_ENABLE      0
#define DFU_TRANSFER_SIZE      64
#define DFU_MANIFEST_REBOOT_LOOPS 12000000u

#define USB_VID                0x1A86u
#define USB_PID                0x8035u
#define USB_STR_MANUF          "CheeseCool"
#define USB_STR_PRODUCT        "CheeseCool DFU"
#define USB_STR_SERIAL         "0001"
#define USB_STR_IFACE          "Application @0x2000"

#endif
