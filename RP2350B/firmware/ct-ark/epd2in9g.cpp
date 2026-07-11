/**
 * Waveshare 2.9inch e-Paper Module(G) 
 * Copyright (c) 2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "tools.h"
#include "Epd2in9g.h"

/**
 * コンストラクタ
 */
Epd2in9g::Epd2in9g()
{
	return;
}

/**
 * デストラクタ
 */
Epd2in9g::~Epd2in9g()
{
	return;
}

void Epd2in9g::initSpi()
{
	// SPI通信に必要な端子の初期化とセットアップ
	spi_init(EPD_SPI, 10 * 1000 * 1000);
	gpio_set_function(EPD_SCLK_PIN,  GPIO_FUNC_SPI);
	gpio_set_function(EPD_MOSI_PIN, GPIO_FUNC_SPI);

	gpio_init(EPD_RST_PIN);
	gpio_init(EPD_DC_PIN);
	gpio_init(EPD_CS_PIN);
	gpio_init(EPD_BUSY_PIN);

	gpio_set_function(EPD_RST_PIN, GPIO_FUNC_SIO);
	gpio_set_function(EPD_DC_PIN, GPIO_FUNC_SIO);
	gpio_set_function(EPD_CS_PIN, GPIO_FUNC_SIO);
	gpio_set_function(EPD_BUSY_PIN, GPIO_FUNC_SIO);

	gpio_set_dir(EPD_RST_PIN,	GPIO_OUT);
	gpio_set_dir(EPD_DC_PIN,	GPIO_OUT);
	gpio_set_dir(EPD_CS_PIN,	GPIO_OUT);
	gpio_set_dir(EPD_BUSY_PIN,	GPIO_IN);

	gpio_disable_pulls(EPD_BUSY_PIN);

	gpio_put(EPD_RST_PIN, 1);	//
	gpio_put(EPD_DC_PIN, 1);	//
	gpio_put(EPD_CS_PIN, 1);	// 非選択

	return;
}

void Epd2in9g::initializeEPD()
{
	const uint8_t initcmds[] = {
		// cmd + datalength + data...
		0x4D,	1,	0x78,					// ??? : サンプルコードにはあるが、UserManual 7.COMMAND TABLEに記載なし。
		0x00,	2,	0x0F,0x29,				// Panel Setting((PSR),
		0x01,	2,	0x07,0x00,				// Power Setting(PWR),
		0x03,	3, 	0x10,0x54,0x44,			// Power of Sequence Setting(PFS),
		0x06,	7,	0x0F,0x0A,0x2F,0x25,	// Booster Soft Start(BTST), 
					0x22,0x2E,0x21,
		0x41,	1,	0x00,					// Tempereture Sensor Command(TSC),
		0x50,	1,	0x37,					// VCOM and DATA interval setting(CDI),
		0x60,	2,	0x02,0x02,				// ??? : サンプルコードにはあるが、UserManual 7.COMMAND TABLEに記載なし。
		0x61,	4,	EPD_2IN9G_WIDTH/256,	// Resolusion setting(TRES),
					EPD_2IN9G_WIDTH%256,	
					EPD_2IN9G_HEIGHT/256,	
					EPD_2IN9G_HEIGHT%256, 	
		0x65,	4,	0x00,0x00,0x00,0x00,	// Gate/Source Start Setting(GSST),
  		0xE7,	1,	0x1C,    				// ??? : サンプルコードにはあるが、UserManual 7.COMMAND TABLEに記載なし。
		0xE3,	1,	0x22,					// Power saving(PWS).
		0xB4,	3,	0xD0,0xB5,0x03,			// ??? : サンプルコードにはあるが、UserManual 7.COMMAND TABLEに記載なし。
		0xE9,	1,	0x01,					// ??? : サンプルコードにはあるが、UserManual 7.COMMAND TABLEに記載なし。
		0x30,	1,	0x08,					// PLL Control(PLL),
		0x04,	0,							// Temperture Senser Command(TSC), ??? これは何のために必要なのだろう？
	};

	const int len = ARRAY_SIZE(initcmds);
	for(int t = 0; t < len; ){
		const uint8_t cmd = initcmds[t++];
		const int lenDt = initcmds[t++];
		sendCommandEPD(cmd);
		t += sendDataEPD(&initcmds[t], lenDt);
	}
	return;
}

void Epd2in9g::resetEPD()
{
	gpio_put(EPD_RST_PIN, 1);
	sleep_ms(200);
	gpio_put(EPD_RST_PIN, 0);
	sleep_ms(2);
	gpio_put(EPD_RST_PIN, 1);
	sleep_ms(200);
	return;
}

void Epd2in9g::waitBusyEPD()
{
	sleep_ms(100);
	while(gpio_get(EPD_BUSY_PIN) != 0) { 
		sleep_ms(100);
	}
	return;
}

void Epd2in9g::waitUnbusyEPD()
{
	sleep_ms(100);
	while(gpio_get(EPD_BUSY_PIN) == 0) { 
		sleep_ms(100);
	}
	return;
}

void Epd2in9g::sendCommandEPD(const uint8_t cmd)
{
	gpio_put(EPD_DC_PIN, 0);	// L=cmd, H=data
	gpio_put(EPD_CS_PIN, 0);
	spi_write_blocking(EPD_SPI, &cmd, 1);
	gpio_put(EPD_CS_PIN, 1);
	return;
}

void Epd2in9g::sendDataEPD(const uint8_t dt)
{
    gpio_put(EPD_DC_PIN, 1);	// L=cmd, H=data
    gpio_put(EPD_CS_PIN, 0);
    spi_write_blocking(EPD_SPI, &dt, 1);
    gpio_put(EPD_CS_PIN, 1);
	return;
}

int Epd2in9g::sendDataEPD(const uint8_t *pDt, const int len)
{
	for(int t = 0; t < len; ++t){
		sendDataEPD(pDt[t]);
	}
	return len;
}

void Epd2in9g::turnOnEPD()
{
    sendCommandEPD(0x12); // DISPLAY_REFRESH
    sendDataEPD(0x00);
    waitUnbusyEPD();
	return;
}

void Epd2in9g::Init()
{
	initSpi();
	resetEPD();

	waitUnbusyEPD();
	initializeEPD();
    sleep_ms(500);
    waitUnbusyEPD(); 
	return;
}

void Epd2in9g::ClearScreen(const uint8_t color)
{
    const int width = (EPD_2IN9G_WIDTH%4 == 0)? (EPD_2IN9G_WIDTH/4 ) : (EPD_2IN9G_WIDTH/4 +1);
    const int height = EPD_2IN9G_HEIGHT;

    sendCommandEPD(0x10);
    gpio_put(EPD_DC_PIN, 1);	// L=cmd, H=data
    gpio_put(EPD_CS_PIN, 0);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
			const uint8_t tile = (color << 6) | (color << 4) | (color << 2) | color;
		    spi_write_blocking(EPD_SPI, &tile, 1);
        }
    }
    gpio_put(EPD_CS_PIN, 1);
    turnOnEPD();
	return;
}

void Epd2in9g::DisplayImage(const uint8_t *pImage)
{
    const int width = (EPD_2IN9G_WIDTH%4 == 0)? (EPD_2IN9G_WIDTH/4 ) : (EPD_2IN9G_WIDTH/4 + 1);
    const int height = EPD_2IN9G_HEIGHT;

    sendCommandEPD(0x10);
    gpio_put(EPD_DC_PIN, 1);	// L=cmd, H=data
    gpio_put(EPD_CS_PIN, 0);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
			const uint8_t tile = pImage[j*width + i];
		    spi_write_blocking(EPD_SPI, &tile, 1);
        }
    }
    gpio_put(EPD_CS_PIN, 1);
    turnOnEPD();
	return;
}

void Epd2in9g::EnterSleep()
{
    sendCommandEPD(0x02); // POWER_OFF
    sendDataEPD(0X00);
    waitUnbusyEPD(); 
    
    sendCommandEPD(0x07); // DEEP_SLEEP
    sendDataEPD(0XA5);
	return;
}

void Epd2in9g::KeepReset()
{
	gpio_put(EPD_RST_PIN, 0);
	return;
}



