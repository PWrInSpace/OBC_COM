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
    uint8_t uartWorkingBuffer[256]; // Zwiększony bufor dla DMA Idle

    // Czas i Data (UTC)
    unsigned short year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    // Status Fixa
    uint8_t fixType;    // 0: no fix, 3: 3D fix
    uint8_t numSV;      // Liczba satelitów użytych w rozwiązaniu (NAV-PVT offset 23)
    
    // Współrzędne (Raw i Float)
    signed long lon;    // stopnie * 1e-7
    signed long lat;    // stopnie * 1e-7
    float fLon;         // stopnie (decimal)
    float fLat;         // stopnie (decimal)

    // Wysokość
    signed long height; // Wysokość nad elipsoidą [mm]
    signed long hMSL;   // Wysokość nad poziomem morza [mm]
    unsigned long hAcc; // Dokładność pozioma [mm]
    unsigned long vAcc; // Dokładność pionowa [mm]

    // Dynamika (Prędkość i Kurs)
    signed long gSpeed;      // Prędkość względem ziemi [mm/s]
    float fGSpeedKmH;        // Prędkość względem ziemi [km/h]
    signed long headMot;     // Kurs poruszania się [deg * 1e-5]
    float fHeadMot;          // Kurs poruszania się [deg]

    // Diagnostyka ramki
    uint8_t checksumCKA;     // Odebrana suma kontrolna A
    uint8_t checksumCKB;     // Odebrana suma kontrolna B
    uint8_t isDataValid;     // Flaga poprawności (1 = checksum OK, 0 = błąd)

} GNSS_StateHandle;

extern GNSS_StateHandle GNSS_Handle;


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



