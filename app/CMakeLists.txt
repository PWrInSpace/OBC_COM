add_library(App STATIC)

target_sources(App
    PRIVATE
        src/sx1280_task.c
        src/rfm95w_task.c
        src/lora_config.c
        src/usb_config.c
        src/cmd_task.c
        src/gps_task.c
        src/sd_task.c
)

target_include_directories(App
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(App
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(App
    PUBLIC
        stm32cubemx
        sx1280
        rfm95w
        sx1280_wrapper
        spi_callbacks
        rfm95w_wrapper
        logger
        USB_Driver
        cmd
        gps
        FatFs
)