add_library(ringBuffer STATIC)

target_sources(ringBuffer
    PRIVATE
        src/ring_buffer.c
)

target_include_directories(ringBuffer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(ringBuffer
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(ringBuffer
    PUBLIC
        stm32cubemx
)