add_library(Proto STATIC
    src/pbtools.c
    src/lora.c
)

target_include_directories(Proto
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

target_compile_options(Proto
    PRIVATE
        -Wno-format
)
