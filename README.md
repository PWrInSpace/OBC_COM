# OBC_COM

![STM32H5](https://img.shields.io/badge/MCU-STM32H563-blue.svg)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20CMSIS--V2-green.svg)
![Build](https://img.shields.io/badge/Build-CMake-orange.svg)

This repository contains the firmware for the **On-Board Computer Communication (OBC_COM)** system. 

The project is built on the **STM32H563** processor, chosen for its high reliability and advanced security features, which are essential requirements for robust communication systems in aerospace applications.

---

## 📂 Project Structure

The codebase is organized into modular layers to ensure scalability and ease of testing:

| Directory | Description |
| :--- | :--- |
| **`app/`** | High-level application logic and FreeRTOS task initializations (CMSIS V2). |
| **`modules/`** | Hardware abstraction drivers for components (RF drivers, GPS, USB, NVS, etc.). |
| **`cmd/`** | Command-line interface logic and instruction processing. |
| **`wrappers/`** | Utility wrappers for simplified hardware interaction. |
| **`Core/`** | Standard STM32 HAL/LL initialization and interrupt handlers. |

---

## 🛠 Setup & Workflow

### 1. Installation
Clone the repository to your local machine:
```bash
git clone [https://github.com/PWrInSpace/OBC_COM.git](https://github.com/PWrInSpace/OBC_COM.git)
