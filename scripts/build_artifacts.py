import os
import shutil

Import("env")

PROJECT_DIR = env.subst("$PROJECT_DIR")
PIOENV = env.subst("$PIOENV")
BUILD_DIR = env.subst("$BUILD_DIR")

def _copy_named(source=None, target=None, env=None, **kwargs):
    firmware_bin = os.path.join(BUILD_DIR, "firmware.bin")
    if PIOENV == "cheesecool_bootloader":
        boot_bin = os.path.join(BUILD_DIR, "bootloader.bin")
        shutil.copyfile(firmware_bin, boot_bin)

        with open(boot_bin, "rb") as boot_file:
            boot_data = boot_file.read()
        if len(boot_data) >= 0x2000:
            raise RuntimeError("bootloader must be smaller than 0x2000 bytes")

        bootloader_only = bytearray([0xFF] * 0xF800)
        bootloader_only[:len(boot_data)] = boot_data
        if bootloader_only[:len(boot_data)] != boot_data:
            raise RuntimeError("bootloader-only image does not start with bootloader data")
        if any(byte != 0xFF for byte in bootloader_only[0x2000:]):
            raise RuntimeError("bootloader-only image application region is not blank")
        if any(byte != 0xFF for byte in bootloader_only[0x2000:0x2100]):
            raise RuntimeError("bootloader-only image application prefix is not blank")

        bootloader_only_bin = os.path.join(BUILD_DIR, "cheesecool-bootloader-only-test.bin")
        with open(bootloader_only_bin, "wb") as image_file:
            image_file.write(bootloader_only)
        print("Bootloader-only image validated: application region is all 0xFF")
        print("  bootloader.bin: %d bytes" % len(boot_data))
        print("  cheesecool-bootloader-only-test.bin: %d bytes" % len(bootloader_only))
        return

    app_bin = os.path.join(BUILD_DIR, "application.bin")
    shutil.copyfile(firmware_bin, app_bin)

    if PIOENV in ("ch32x035f8u6_evt_r0_dfu_test", "ch32x035f8u6_evt_r0_usb_diag"):
        print("DFU Test application artifact generated; factory image generation skipped")
        return

    boot_bin = os.path.join(PROJECT_DIR, ".pio", "build", "cheesecool_bootloader", "bootloader.bin")
    if PIOENV == "ch32x035f8u6_evt_r0_dfu_test":
        factory_name = "cheesecool-factory-dfu-test.bin"
    elif PIOENV == "reference_min_app":
        factory_name = "cheesecool-factory-reference-minapp.bin"
    elif PIOENV == "dfu_handoff_diag":
        factory_name = "cheesecool-factory-dfu-handoff-diag.bin"
    elif PIOENV == "initialization_bisect_diag":
        factory_name = "cheesecool-factory-init-bisect.bin"
    elif PIOENV == "fan_controller_diag":
        factory_name = "cheesecool-factory-fan-controller-diag.bin"
    else:
        factory_name = "cheesecool-factory.bin"
    factory_bin = os.path.join(BUILD_DIR, factory_name)
    if not os.path.isfile(boot_bin):
        print("Warning: bootloader.bin is not available; factory image not generated")
        return

    flash_image = bytearray([0xFF] * 0xF800)
    with open(boot_bin, "rb") as boot_file:
        boot_data = boot_file.read()
    with open(app_bin, "rb") as app_file:
        app_data = app_file.read()
    if len(boot_data) > 0x2000:
        raise RuntimeError("bootloader exceeds 8 KiB reserved region")
    if len(app_data) > 0xD800:
        raise RuntimeError("application exceeds 54 KiB reserved region")
    flash_image[0:len(boot_data)] = boot_data
    flash_image[0x2000:0x2000 + len(app_data)] = app_data

    if len(flash_image) != 0xF800:
        raise RuntimeError("factory image has an invalid total size")
    if flash_image[:len(boot_data)] != boot_data:
        raise RuntimeError("bootloader is not at factory image offset 0x0000")
    if any(byte != 0xFF for byte in flash_image[len(boot_data):0x2000]):
        raise RuntimeError("factory image contains non-padding data before application offset 0x2000")
    if flash_image[0x2000:0x2000 + len(app_data)] != app_data:
        raise RuntimeError("application is not at factory image offset 0x2000")
    if any(byte != 0xFF for byte in flash_image[0x2000 + len(app_data):]):
        raise RuntimeError("factory image contains data beyond the application payload")

    with open(factory_bin, "wb") as factory_file:
        factory_file.write(flash_image)
    print("Factory image validated: bootloader @ 0x0000, application @ 0x2000")
    print("  bootloader.bin: %d bytes" % len(boot_data))
    print("  application.bin: %d bytes" % len(app_data))
    print("  %s: %d bytes" % (factory_name, len(flash_image)))

env.AddPostAction("$BUILD_DIR/firmware.bin", _copy_named)
