add_library(rfm95w_wrapper STATIC)

target_sources(rfm95w_wrapper
    PRIVATE
        src/rfm95w_wrapper.c
)

target_include_directories(rfm95w_wrapper
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(rfm95w_wrapper
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(rfm95w_wrapper
    PUBLIC
        rfm95w
        stm32cubemx
)

