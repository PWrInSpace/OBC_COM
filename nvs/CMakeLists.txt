add_library(Nvs STATIC)

target_sources(Nvs
    PRIVATE
        src/nvs_config.c
        src/flash_interface.c
        src/eeprom_emul.c
)

target_include_directories(Nvs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_compile_options(Nvs
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(Nvs
    PUBLIC
        stm32cubemx
        sx1280
        rfm95w
        sx1280_wrapper
        spi_callbacks
        rfm95w_wrapper
        logger
        USB_Driver
)