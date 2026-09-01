Import("env")

env.BuildSources(
    "$BUILD_DIR/InitializationBisectDiagSources",
    "$PROJECT_DIR/initialization_bisect_diag",
)
env.BuildSources(
    "$BUILD_DIR/InitializationBisectFirmwareSources",
    "$PROJECT_DIR/src",
    "-<*> +<debug_test.c> +<failsafe.c> +<fan_controller.c> +<fan_pwm.c> "
    "+<fan_tach.c> +<power_monitor.c> +<system_status.c> +<usb_device.c> +<usb_protocol.c>",
)
