# OBC_COM

![MCU](https://img.shields.io/badge/MCU-STM32H563-blue.svg)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20CMSIS--V2-green.svg)
![Build](https://img.shields.io/badge/Build-CMake-orange.svg)

Firmware for the On-Board Computer (OBC) communication subsystem. This project is based on the **STM32H563** processor, chosen for its high reliability and integrated security features—essential requirements for robust communication systems in aerospace.

---

## 📂 Project Structure

| Directory | Description |
| :--- | :--- |
| **`app/`** | High-level application logic and FreeRTOS task initializations (CMSIS V2). |
| **`modules/`** | Component drivers (RF_driver, GPS, USB, logger, NVS, etc.). |
| **`cmd/`** | Command handling and instruction parsing logic. |
| **`wrappers/`** | Hardware abstraction layers for simplified peripheral access. |

---

## 🛠 VS Code Automation (tasks.json)

The repository is configured to streamline the flashing process using the **`CTRL + SHIFT + B`** shortcut.

### Required Configuration:
1. Open the `.vscode/tasks.json` file.
2. **Programmer Path**: Ensure the path to the `STM32_Programmer_CLI` matches your local **STM32CubeProgrammer** installation.
3. **COM Port**: Set the specific **COM port** (e.g., `COM3`) assigned to your device.

> [!IMPORTANT]  
> **Build before flashing:** The flash task targets the **`.bin`** file. You must successfully build the project before flashing to ensure the binary is up-to-date.

---

## 🚀 Flashing Instructions (USB DFU Mode)

The board uses USB DFU for firmware updates. Follow these steps to toggle between programming and execution modes:

### Step 1: Programming Mode (Bootloader)
1. Set the **BOOT0** jumper/switch to **GND**.
2. Press the **RESET (RST)** button.
3. In VS Code, press **`CTRL + SHIFT + B`**. The system will automatically upload the binary via the configured COM port.

### Step 2: Execution Mode (Run)
1. Once the upload is complete, move the **BOOT0** switch to **3V3 (VCC)**.
2. Press the **RESET (RST)** button again.
3. The firmware will now start executing from the Flash memory.

## 🛠 STM32CubeMX
1. After STM32CubeMX code generation chnage the cmake/stm32cubemx/Cmakelists.txt file as follows:
```c
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    RTOS2
    "-u _printf_float"
	
)```

## 🏗 Building the Project
This project uses **CMake**. It is recommended to use the *CMake Tools* extension in VS Code. Simply select your kit, configure the project, and build to generate the necessary `.bin` files for flashing.
