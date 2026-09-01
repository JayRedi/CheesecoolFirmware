Import("env")

env.BuildSources(
    "$BUILD_DIR/FanControllerDiagSources",
    "$PROJECT_DIR/fan_controller_diag",
)
env.BuildSources(
    "$BUILD_DIR/FanControllerDiagFirmwareSources",
    "$PROJECT_DIR/src",
    "-<*> +<fan_controller.c> +<fan_pwm.c>",
)
