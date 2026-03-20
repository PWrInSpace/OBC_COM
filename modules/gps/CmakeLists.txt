# Definicja biblioteki
add_library(gps STATIC 
    Src/GNSS.c
)

target_compile_options(gps
    PRIVATE
        -Wall
        -Wextra
)


# Podajemy ścieżki do nagłówków, których GNSS.c potrzebuje do kompilacji
target_include_directories(gps PUBLIC 
    Inc
    ../../Core/Inc
    ../../Drivers/STM32H5xx_HAL_Driver/Inc
    ../../Drivers/CMSIS/Device/ST/STM32H5xx/Include
    ../../Drivers/CMSIS/Include
    ../../Middlewares/Third_Party/FreeRTOS/Source/include
    ../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM33_NTZ/non_secure
)

# Dodaj definicje procesora, bez nich HAL wywala błędy w main.h
target_compile_definitions(gps PUBLIC STM32H563xx)
