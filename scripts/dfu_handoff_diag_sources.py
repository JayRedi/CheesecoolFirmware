Import("env")

env.BuildSources(
    "$BUILD_DIR/DfuHandoffDiagSources",
    "$PROJECT_DIR/dfu_handoff_diag",
)
env.BuildSources(
    "$BUILD_DIR/DfuHandoffDiagFirmwareSources",
    "$PROJECT_DIR/src",
    "-<*> +<system_dfu.c> +<fan_controller.c> +<fan_pwm.c>",
)
