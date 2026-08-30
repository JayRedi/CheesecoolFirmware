Import("env")

env.BuildSources(
    "$BUILD_DIR/SystemMillisDiagSources",
    "$PROJECT_DIR/system_millis_diag",
)
env.BuildSources(
    "$BUILD_DIR/SystemMillisDiagFirmwareSources",
    "$PROJECT_DIR/src",
    "-<*> +<fan_controller.c> +<fan_pwm.c> +<system_dfu.c>",
)
