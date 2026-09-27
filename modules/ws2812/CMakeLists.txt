add_library(ws2812 STATIC)

target_sources(ws2812
    PRIVATE
        Src/ws2812.c
)

target_include_directories(ws2812
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_compile_options(ws2812
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(ws2812
    PUBLIC
        stm32cubemx
)
