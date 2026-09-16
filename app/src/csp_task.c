/*
 * Author: Mateusz Kłosiński
 * Organization: PWr in Space
 * Date: 16.09.2026
 */
#include <stdio.h>

#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/autoconfig.h>

#include "cmsis_os2.h"
#include "csp_task.h"

#define TAG "CSP"
#include "logger.h"

/* Loopback-only sanity demo: client and server both run on this node (address 0). */
#define CSP_DEMO_ADDRESS	0
#define CSP_DEMO_PORT		10

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
			if (csp_conn_dport(conn) == CSP_DEMO_PORT) {
				LOG_INFO("Packet received on CSP_DEMO_PORT: %s", (char *) packet->data);
				csp_buffer_free(packet);
			} else {
				csp_service_handler(packet);
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

	cspRouterTaskHandle = osThreadNew(csp_router_task, NULL, &cspRouterTask_attributes);
	cspServerTaskHandle = osThreadNew(csp_server_task, NULL, &cspServerTask_attributes);
	cspClientTaskHandle = osThreadNew(csp_client_task, NULL, &cspClientTask_attributes);
}
