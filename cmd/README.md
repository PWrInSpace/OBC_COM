# CMD Module (Command Line Interface)

The **CMD Module** provides a robust interface for controlling the OBC via USB or Radio. It is designed to be highly extensible, supporting both **Human-Readable (ASCII)** and **Binary** communication protocols.

---

## ⚙️ Protocol Architecture

The module automatically detects the incoming packet type based on the initial bytes:

### 1. Text Protocol (ASCII)
Designed for manual debugging via serial terminal (e.g., PuTTY, TeraTerm).
* **Format:** `CMD:[COMMAND]:[VALUE]`
* **Example:** `CMD:FREQ:868000000` (Sets frequency to 868 MHz)

### 2. Binary Protocol
Designed for automated communication between ground stations or other flight computers.
* **Format:** `[Header 0x32][Data Length][Command ID][Payload...]`
* **Example:** `32 04 01 00 CA 9A 3B` (Binary equivalent of setting frequency)

---

## 📜 Supported Commands

| Command | ID (Hex) | Description | Example (Text) |
| :--- | :--- | :--- | :--- |
| **`HELP`** | `0x00` | Displays the help menu with ID and descriptions. | `CMD:HELP` |
| **`FREQ`** | `0x01` | Sets the radio frequency (in Hz). | `CMD:FREQ:868000000` |
| **`POWER`** | `0x02` | Sets TX power in dBm. | `CMD:POWER:14` |
| **`STATUS`**| `0x04` | Returns current radio link and system status. | `CMD:STATUS` |
| **`LORATX`**| `0x06` | Queues data to be sent via RFM95W radio. | `CMD:LORATX:DATA` |
| **`LOGON`** | `0x07` | Enables all logs and saves preference to NVS. | `CMD:LOGON` |
| **`LOGOFF`**| `0x08` | Disables all logs and saves preference to NVS. | `CMD:LOGOFF` |
| **`LOGMUTE`**| `0x09` | Mutes logs for a specific TAG (e.g., GPS). | `CMD:LOGMUTE:GPS` |
| **`RESET`** | `0x03` | Triggers a hardware `NVIC_SystemReset()`. | `CMD:RESET` |

---

## 🛠 Developer Guide: Adding New Commands

To add a new command to the system, follow these three steps:

### Step 1: Define Command ID
Open `cmd_interface.h` and add your new command to the `Command_t` enum. This ID is mandatory for binary protocol support.
```c
typedef enum {
    CMD_HELP = 0x00,
    // ...
    CMD_MY_NEW_COMMAND = 0x0A, // Your new ID here
    CMD_NUM
} Command_t;

### Step 2: Define Command Struct
add in CommandMap_t cmd_map[] the command with handler etc...

static const CommandMap_t cmd_map[] = {
    // ...
    {"CMD_NAME", CMD_MY_NEW_ACTION, my__new_handler, "- Description for help menu"},
};

### Step 3: Define Command Handler

void my_new_handler(cmd_params_t *params) {

}
