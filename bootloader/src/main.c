#include "ch32x035.h"
#include "config.h"

void dfu_run(void);

int main(void)
{
    volatile uint32_t *flag = (volatile uint32_t *)BOOT_FLAG_ADDR;
    int want_dfu = (*flag == BOOT_MAGIC_DFU);
    *flag = 0;

    uint32_t first = *(volatile uint32_t *)APP_BASE;
    int app_valid = (first != 0xFFFFFFFFu && first != 0x00000000u);

#if DFU_ENTER_IF_NO_APP
    if (!app_valid)
        want_dfu = 1;
#endif

    if (!want_dfu && app_valid) {
        void (*app_entry)(void) = (void (*)(void))APP_BASE;
        app_entry();
    }

    dfu_run();
    while (1) { }
}
