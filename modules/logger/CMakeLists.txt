add_library(logger STATIC)

target_sources(logger
    PRIVATE
        src/logger.c
        src/logger_serial_output.c
)

target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(logger
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(logger
    PUBLIC
        stm32cubemx
        USB_Driver
        App
)