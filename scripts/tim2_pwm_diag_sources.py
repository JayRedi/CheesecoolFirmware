Import("env")

env.BuildSources(
    "$BUILD_DIR/Tim2PwmDiagSources",
    "$PROJECT_DIR/pwm_init_survival_diag",
    "-<*> +<tim2_pwm_diag_main.c>",
)
env.BuildSources(
    "$BUILD_DIR/Tim2PwmDiagDfuSource",
    "$PROJECT_DIR/src",
    "-<*> +<system_dfu.c>",
)
