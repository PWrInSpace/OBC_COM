add_library(spi_callbacks STATIC)

target_sources(spi_callbacks
    PRIVATE
        src/spi_callbacks.c
)

target_include_directories(spi_callbacks
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(spi_callbacks
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(spi_callbacks
    PUBLIC
        stm32cubemx
)