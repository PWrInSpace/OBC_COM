# OBC_COM {#mainpage}

![MCU](https://img.shields.io/badge/MCU-STM32H563-blue.svg)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20CMSIS--V2-green.svg)
![Build](https://img.shields.io/badge/Build-CMake-orange.svg)
![Code Style](https://img.shields.io/badge/Code_Style-Clang--Format-blueviolet.svg)

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

## 🛠 VS Code Automation & Build Process

The repository is configured to streamline development and enforce code quality using automated VS Code tasks (`.vscode/tasks.json`).

*   **Format & Build (`CTRL + SHIFT + B`)**: Pressing this shortcut will automatically run `clang-format` to style your C/C++ files, and then build the project using CMake.
*   **Flash Firmware**: To upload the code, go to **Terminal -> Run Task...** in the top menu and select **`Flash DFU`**.

### Required Configuration for Flashing:
Before flashing for the first time, check the `Flash DFU` task in `.vscode/tasks.json`:
1. **Programmer Path**: Ensure the path to the `STM32_Programmer_CLI` matches your local **STM32CubeProgrammer** installation.
2. **Port**: Set the specific port (e.g., `port=usb1` or `COM3`) assigned to your device in the `args` array.

> [!IMPORTANT]  
> **Build before flashing:** The flash task targets the **`.bin`** file. You must successfully build the project (`CTRL + SHIFT + B`) before flashing to ensure the binary is up-to-date.

---

## 🚀 Flashing Instructions (USB DFU Mode)

The board uses USB DFU for firmware updates. Follow these steps to toggle between programming and execution modes:

### Step 1: Programming Mode (Bootloader)
1. Set the **BOOT0** jumper/switch to **GND**.
2. Press the **RESET (RST)** button.
3. In VS Code, navigate to **Terminal -> Run Task...** and select **`Flash DFU`**. The system will automatically upload the binary.

### Step 2: Execution Mode (Run)
1. Once the upload is complete, move the **BOOT0** switch to **3V3 (VCC)**.
2. Press the **RESET (RST)** button again.
3. The firmware will now start executing from the Flash memory.

---

## 🛠 STM32CubeMX Notes
1. After STM32CubeMX code generation, change the `cmake/stm32cubemx/CMakeLists.txt` file as follows:
```cmake
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    RTOS2
    "-u _printf_float"
)
```
