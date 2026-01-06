add_library(LoraUtils STATIC)

target_sources(LoraUtils
    PRIVATE
        src/lora_utilities.c
        src/lora_commands.c
)

target_include_directories(LoraUtils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(LoraUtils
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(LoraUtils
    PUBLIC
        stm32cubemx
        sx1280
)