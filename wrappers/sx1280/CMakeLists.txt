add_library(sx1280_wrapper STATIC)

target_sources(sx1280_wrapper
    PRIVATE
        src/sx1280_hal_wrapper.c
)

target_include_directories(sx1280_wrapper
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(sx1280_wrapper
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(sx1280_wrapper
    PUBLIC
        sx1280
        stm32cubemx
)