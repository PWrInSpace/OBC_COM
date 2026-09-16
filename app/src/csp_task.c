/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 16.09.2026
 */
#include <stdio.h>
#include <string.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/autoconfig.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "csp_task.h"
#include "usb_config.h"
#include "cmd_interface.h"
#include "main.h"

#define TAG "CSP"
#include "logger.h"

/* Loopback-only sanity demo: client and server both run on this node (address 0). */
#define CSP_DEMO_ADDRESS	0
#define CSP_DEMO_PORT		10

/* Packets arriving on this port (typically from the ground station over USB) get
 * their payload forwarded to the RFM95W module for transmission over LoRa. */
#define CSP_LORA_FWD_PORT	11

extern osThreadId_t rfm95wTaskHandle;

static csp_iface_t csp_usb_iface;
static csp_kiss_interface_data_t csp_usb_ifdata;

/* Serializes KISS-framed transmissions on the USB link across tasks; libcsp's generic
 * csp_kiss_tx() calls these unconditionally regardless of which driver is attached. */
static osMutexId_t csp_usb_tx_mutex = NULL;

void csp_usart_lock(void *driver_data) {
	(void) driver_data;
	if (csp_usb_tx_mutex != NULL) {
		osMutexAcquire(csp_usb_tx_mutex, osWaitForever);
	}
}

void csp_usart_unlock(void *driver_data) {
	(void) driver_data;
	if (csp_usb_tx_mutex != NULL) {
		osMutexRelease(csp_usb_tx_mutex);
	}
}

static osThreadId_t cspRouterTaskHandle = NULL;
static const osThreadAttr_t cspRouterTask_attributes = {
	.name = "cspRouterTask",
	.stack_size = 1024,
	.priority = (osPriority_t) osPriorityAboveNormal,
};

static osThreadId_t cspServerTaskHandle = NULL;
static const osThreadAttr_t cspServerTask_attributes = {
	.name = "cspServerTask",
	.stack_size = 1024,
	.priority = (osPriority_t) osPriorityNormal,
};

static osThreadId_t cspClientTaskHandle = NULL;
static const osThreadAttr_t cspClientTask_attributes = {
	.name = "cspClientTask",
	.stack_size = 1024,
	.priority = (osPriority_t) osPriorityNormal,
};

/* CSP/KISS tx callback: hand the framed bytes to the existing USB CDC transmitter. */
static int csp_usb_kiss_tx(void *driver_data, const uint8_t *data, size_t len) {
	(void) driver_data;
	USB_Transmit((uint8_t *) data, (uint16_t) len);
	return CSP_ERR_NONE;
}

void CSP_USB_Kiss_Feed(const uint8_t *data, size_t len) {
	csp_kiss_rx(&csp_usb_iface, data, len, NULL);
}

/* Hand a packet's payload to the RFM95W task for transmission, same handoff used by handle_lora_tx(). */
static void csp_forward_to_lora(csp_packet_t *packet) {
	uint16_t copy_len = (packet->length < LORA_BUFF_SIZE) ? packet->length : LORA_BUFF_SIZE;
	memcpy(LoraRxBuffer, packet->data, copy_len);
	lora_cmd_len = copy_len;

	if (rfm95wTaskHandle != NULL) {
		xTaskNotify(rfm95wTaskHandle, LORA_TX_EVENT_BIT, eSetBits);
	}
}

/* Moves packets out of the interface queues and into connections; libcsp has no built-in router thread. */
static void csp_router_task(void *argument) {
	(void) argument;

	for (;;) {
		csp_route_work();
	}
}

static void csp_server_task(void *argument) {
	(void) argument;

	csp_socket_t sock = {0};
	csp_bind(&sock, CSP_ANY);
	csp_listen(&sock, 10);

	for (;;) {
		csp_conn_t *conn = csp_accept(&sock, 10000);
		if (conn == NULL) {
			continue;
		}

		csp_packet_t *packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case CSP_DEMO_PORT:
				LOG_INFO("Packet received on CSP_DEMO_PORT: %s", (char *) packet->data);
				csp_buffer_free(packet);
				break;
			case CSP_LORA_FWD_PORT:
				csp_forward_to_lora(packet);
				csp_buffer_free(packet);
				break;
			default:
				csp_service_handler(packet);
				break;
			}
		}

		csp_close(conn);
	}
}

static void csp_client_task(void *argument) {
	(void) argument;

	uint8_t count = 'A';

	for (;;) {
		osDelay(1000);

		int result = csp_ping(CSP_DEMO_ADDRESS, 1000, 100, CSP_O_NONE);
		LOG_INFO("Ping address: %u, result %d [mS]", CSP_DEMO_ADDRESS, result);

		csp_conn_t *conn = csp_connect(CSP_PRIO_NORM, CSP_DEMO_ADDRESS, CSP_DEMO_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			LOG_ERROR("Connection failed");
			continue;
		}

		csp_packet_t *packet = csp_buffer_get(0);
		if (packet == NULL) {
			LOG_ERROR("Failed to get CSP buffer");
			csp_close(conn);
			continue;
		}

		int msg_len = snprintf((char *) packet->data, CSP_BUFFER_SIZE, "Hello world %c", count++);
		packet->length = (uint16_t) (msg_len + 1);

		csp_send(conn, packet);
		csp_close(conn);
	}
}

void CSP_Task_Init(void) {
	csp_init();

	csp_usb_tx_mutex = osMutexNew(NULL);

	csp_usb_iface.name = "USB";
	csp_usb_iface.addr = CSP_DEMO_ADDRESS;
	csp_usb_iface.interface_data = &csp_usb_ifdata;
	csp_usb_ifdata.tx_func = csp_usb_kiss_tx;
	csp_kiss_add_interface(&csp_usb_iface);

	cspRouterTaskHandle = osThreadNew(csp_router_task, NULL, &cspRouterTask_attributes);
	cspServerTaskHandle = osThreadNew(csp_server_task, NULL, &cspServerTask_attributes);
	cspClientTaskHandle = osThreadNew(csp_client_task, NULL, &cspClientTask_attributes);
}
