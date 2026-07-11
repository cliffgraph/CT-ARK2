/**
 * Waveshare 2.9inch e-Paper Module(G) 
 * Copyright (c) 2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/


#include <stdio.h>
#include <stdint.h>		// for uint8_t


// Display resolution
#define	EPD_2IN9G_WIDTH			128
#define	EPD_2IN9G_HEIGHT		296
// Color
#define	EPD_2IN9G_BLACK   		0x0
#define	EPD_2IN9G_WHITE   		0x1
#define	EPD_2IN9G_YELLOW  		0x2
#define	EPD_2IN9G_RED     		0x3

//
#define EPD_SPI			spi1
#define	EPD_RST_PIN		39
#define	EPD_DC_PIN		40
#define	EPD_BUSY_PIN	38
#define	EPD_CS_PIN		41
#define	EPD_SCLK_PIN	42
#define	EPD_MOSI_PIN	43

class Epd2in9g
{
public:
	Epd2in9g();
	virtual ~Epd2in9g();
private:
	void initSpi();
	void initializeEPD();
	void resetEPD();
	void waitBusyEPD();
	void waitUnbusyEPD();
	void sendCommandEPD(const uint8_t cmd);
	void sendDataEPD(const uint8_t dt);
	int sendDataEPD(const uint8_t *pDt, const int len);
	void turnOnEPD();
public:
	void Init();
	void ClearScreen(const uint8_t color);
	void DisplayImage(const uint8_t *pImage);
	void EnterSleep();
	void KeepReset();
};



