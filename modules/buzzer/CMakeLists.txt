add_library(buzzer STATIC)

target_sources(buzzer
    PRIVATE
        Src/buzzer.c
)

target_include_directories(buzzer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../app/include
)

target_compile_options(buzzer
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(buzzer
    PUBLIC
        stm32cubemx
        logger
)
