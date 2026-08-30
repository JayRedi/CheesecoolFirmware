#include <stdio.h>
#include <libusb.h>

int main(void)
{
    libusb_context *ctx = NULL;
    libusb_device **list = NULL;
    ssize_t count;
    int rc = libusb_init(&ctx);
    if (rc < 0) {
        fprintf(stderr, "libusb_init=%d (%s)\n", rc, libusb_error_name(rc));
        return 1;
    }
    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    count = libusb_get_device_list(ctx, &list);
    if (count < 0) {
        fprintf(stderr, "libusb_get_device_list=%zd (%s)\n", count, libusb_error_name((int)count));
        libusb_exit(ctx);
        return 2;
    }
    printf("device_count=%zd\n", count);
    for (ssize_t i = 0; i < count; ++i) {
        struct libusb_device_descriptor d;
        rc = libusb_get_device_descriptor(list[i], &d);
        if (rc < 0) {
            printf("index=%zd descriptor=%d (%s)\n", i, rc, libusb_error_name(rc));
            continue;
        }
        if (d.idVendor == 0x1a86 || d.idProduct == 0x8035) {
            printf("index=%zd vid=%04x pid=%04x class=%02x subclass=%02x protocol=%02x configs=%u\n",
                   i, d.idVendor, d.idProduct, d.bDeviceClass, d.bDeviceSubClass,
                   d.bDeviceProtocol, d.bNumConfigurations);
        }
    }
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    return 0;
}
