 /*
 * Mateusz Kłosiński Library - GPS Module MAX-M10S 20.03.2026r.
 ******************************************************************************
 */

#ifndef INC_GNSS_H_
#define INC_GNSS_H_

#include "main.h"

union u_Short
{
	uint8_t bytes[2];
	unsigned short uShort;
};

union i_Short
{
	uint8_t bytes[2];
	signed short iShort;
};

union u_Long
{
	uint8_t bytes[4];
	unsigned long uLong;
};

union i_Long
{
	uint8_t bytes[4];
	signed long iLong;
};

typedef struct
{
	UART_HandleTypeDef *huart;

	uint8_t uniqueID[6];
	uint8_t uartWorkingBuffer[101];

	unsigned short year;
	uint8_t yearBytes[2];
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t fixType;

	signed long lon;
	uint8_t lonBytes[4];
	signed long lat;
	uint8_t latBytes[4];
	float fLon;
	float fLat;

	signed long height;
	signed long hMSL;
	uint8_t hMSLBytes[4];
	unsigned long hAcc;
	unsigned long vAcc;

	signed long gSpeed;
	uint8_t gSpeedBytes[4];
	signed long headMot;
	float fGSpeedKmH;

}GNSS_StateHandle;

extern GNSS_StateHandle GNSS_Handle;

// 1. Wyłączenie NMEA na UART1 (aby nie śmieciło w DMA)
static const uint8_t disableNmeaUart1[] = {0xB5, 0x62, 0x06, 0x8B, 0x09, 0x00, 0x01, 0x01, 0x00, 0x00, 0x20, 0x91, 0x01, 0x00, 0x00, 0x46, 0x51};

// 2. Model Dynamiczny AIR4 (Airborne < 4g) - KRYTYCZNE DLA RAKIETY
// Pozwala na pracę przy dużych prędkościach i wysokościach (Klucz: 0x20110021, Wartość: 8)
static const uint8_t setRocketMode4G[] = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x21, 0x00, 0x11, 0x20, 0x08, 0x94, 0xB7};

//!:PRZY STARCIE ZMIENIĆ NA 10Hz (komenda z MCB)
// 3. Ustawienie 1Hz (1000ms) - system VALSET dla M10
static const uint8_t setRate1Hz_M10[] = {0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x21, 0x30, 0xE8, 0x03, 0xF1, 0xAE};

// 4. Włączenie wiadomości binarnej NAV-PVT (Pozycja, Czas, Prędkość)
static const uint8_t enableNavPvt[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x18, 0xE1};

// Komenda zapytania o ID (UBX-SEC-UNIQID): Class 0x27, ID 0x03
static const uint8_t getDeviceID_M10[] = {0xB5, 0x62, 0x27, 0x03, 0x00, 0x00, 0x2A, 0xA5};



enum GNSSMode{Portable=0, Stationary=1, Pedestrian=2, Automotiv=3, Airbone1G=5, Airbone2G=6,Airbone4G=7,Wirst=8,Bike=9};

static const uint8_t setPortableType[]={};
void GNSS_Init(GNSS_StateHandle *GNSS, UART_HandleTypeDef *huart);
void GNSS_LoadConfig(GNSS_StateHandle *GNSS);
void GNSS_ParseBuffer(GNSS_StateHandle *GNSS);

void GNSS_GetUniqID(GNSS_StateHandle *GNSS);
void GNSS_ParseUniqID(GNSS_StateHandle *GNSS);

void GNSS_GetNavigatorData(GNSS_StateHandle *GNSS);
void GNSS_ParseNavigatorData(GNSS_StateHandle *GNSS);

void GNSS_GetPOSLLHData(GNSS_StateHandle *GNSS);
void GNSS_ParsePOSLLHData(GNSS_StateHandle *GNSS);

void GNSS_GetPVTData(GNSS_StateHandle *GNSS);
void GNSS_ParsePVTData(GNSS_StateHandle *GNSS);

void GNSS_SetMode(GNSS_StateHandle *GNSS, short gnssMode);
#endif /* INC_GNSS_H_ */



