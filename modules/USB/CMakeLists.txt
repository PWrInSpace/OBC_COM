add_library(USB_Driver STATIC)

target_sources(USB_Driver
    PRIVATE
        Class/CDC/Src/usbd_cdc_if.c
        Class/CDC/Src/usbd_cdc.c
        Core/Src/usbd_desc.c
        Core/Src/usbd_core.c
        Core/Src/usbd_ctlreq.c
        Core/Src/usbd_ioreq.c
        Core/Src/usbd_conf.c
)

target_include_directories(USB_Driver
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Class/CDC/Inc
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

target_compile_options(USB_Driver
    PRIVATE
        -Wall
        -Wextra
)

target_link_libraries(USB_Driver
    PUBLIC
        stm32cubemx
)