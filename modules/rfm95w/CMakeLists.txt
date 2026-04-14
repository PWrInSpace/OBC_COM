add_library(rfm95w STATIC)

target_sources(rfm95w
    PRIVATE
        src/rfm95w.c
)

target_include_directories(rfm95w
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../app/include
)

target_compile_options(rfm95w
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(rfm95w
    PUBLIC
        USB_Driver
        
)