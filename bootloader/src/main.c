#include "ch32x035.h"
#include "config.h"

void dfu_run(void);

int main(void)
{
    uint32_t first = *(volatile uint32_t *)APP_BASE;
    int app_valid = (first != 0xFFFFFFFFu && first != 0x00000000u);

    if (app_valid) {
        void (*app_entry)(void) = (void (*)(void))APP_BASE;
        app_entry();
    }

#if DFU_ENTER_IF_NO_APP
    dfu_run();
#else
    while (1) { }
#endif
}
