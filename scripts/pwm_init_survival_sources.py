Import("env")

env.BuildSources(
    "$BUILD_DIR/PwmInitSurvivalDiagSources",
    "$PROJECT_DIR/pwm_init_survival_diag",
)
env.BuildSources(
    "$BUILD_DIR/PwmInitSurvivalDiagFirmwareSources",
    "$PROJECT_DIR/src",
    "-<*> +<fan_controller.c> +<fan_pwm.c>",
)
