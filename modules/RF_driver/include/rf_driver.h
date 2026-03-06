/*
 * Author: Szymon Rzewuski
 * Organization: PWr in Space
 * Date: 27.01.2026
 */

#pragma once


/*Low noise amplifier*/
typedef struct LNA_driver{
    GPIO_TypeDef * GPIOx;
    uint16_t Pin;
} LNA_Driver_t;

/*RF switch*/
typedef struct Switch_driver{
    GPIO_TypeDef * aGPIOx;
    uint16_t aPin;
    GPIO_TypeDef * bGPIOx;
    uint16_t bPin;
} Switch_Driver_t;

/*Power Amplifier*/
typedef struct PA_driver{
    GPIO_TypeDef * enableGPIOx;
    uint16_t enablePin;
    GPIO_TypeDef * detectGPIOx;
    uint16_t detectPin;
    GPIO_TypeDef * pwrSenseGPIOx;
    uint16_t pwrSensePin;
} PA_Driver_t;

typedef struct RF_driver{
    // RF driver related members
} RF_Driver_t;