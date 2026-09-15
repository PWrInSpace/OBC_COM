add_library(sky66114 STATIC)

target_sources(sky66114
    PRIVATE
        Src/sky66114.c
)

target_include_directories(sky66114
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../app/include
)

target_compile_options(sky66114         
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(sky66114
    PUBLIC
        USB_Driver
        logger
        
)