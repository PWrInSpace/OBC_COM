
add_library(sx1280_wrapper STATIC
    src/sx1280_hal_wrapper.c
)

target_link_libraries(sx1280_wrapper
    PUBLIC
        sx1280
        stm32cubemx
)