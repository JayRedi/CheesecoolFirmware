Import("env")

env.BuildSources(
    "$BUILD_DIR/BootloaderSources",
    "$PROJECT_DIR/bootloader/src"
)
