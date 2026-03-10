add_library(sx1280 STATIC)

target_sources(sx1280
    PRIVATE
        src/sx1280.c
        src/sx1280-hal.c
)

target_include_directories(sx1280
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(sx1280
    PRIVATE
        -Wall
        -Wextra
)
