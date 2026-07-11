/**
 * CT-ARK (RaspberryPiPico firmware)
 * Copyright (c) 2023 Harumakkin.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hardware/spi.h"

const uint32_t GP_D0						= 0;
const uint32_t GP_D1						= 1;
const uint32_t GP_D2						= 2;
const uint32_t GP_D3						= 3;
const uint32_t GP_D4						= 4;
const uint32_t GP_D5						= 5;
const uint32_t GP_D6						= 6;
const uint32_t GP_D7						= 7;
//
const uint32_t GP_A0						= 8;
const uint32_t GP_A1						= 9;
const uint32_t GP_A2						= 10;
const uint32_t GP_A3						= 11;
const uint32_t GP_A4						= 12;
const uint32_t GP_A5						= 13;
const uint32_t GP_A6						= 14;
const uint32_t GP_A7						= 15;
const uint32_t GP_A8						= 16;
const uint32_t GP_A9						= 17;
const uint32_t GP_A10						= 18;
const uint32_t GP_A11						= 19;
const uint32_t GP_A12						= 20;
const uint32_t GP_A13						= 21;
const uint32_t GP_A14						= 22;
const uint32_t GP_A15						= 23;
//
#ifdef WAVESHARE_CORE2350B
const uint32_t GP_PICO_LED					= 46;	// WAVESHARE_CORE2350B: LED Bkue
//
const uint32_t GP_SIRW						= 28;
const uint32_t GP_SLTSL						= 27;
const uint32_t GP_IOREQ						= 26;
const uint32_t GP_WR						= 25;
const uint32_t GP_RD						= 24;
const uint32_t GP_D0_DIR					= 31;
const uint32_t GP_SLIO						= 29;
//
const uint32_t GP_WAIT						= 37;
const uint32_t GP_EXT_OUT_PIN2				= 44;	// J2
const uint32_t GP_EXT_OUT_PIN				= 45;	// J1
const uint32_t GP_MODE_SW					= 32;	// pulluped, H=Cartridge mode, L=Reader Mode
//
// [x][/CS12][/SLIO][/SIRW], [/SLTSL][/IORQ][/WR][/RD], [A15-A8], [A7-A0], [D7-D0]
const uint32_t MSXCTRL_MASK		= 0x0F000000;		// [/SLTSL][/IORQ][/WR][/RD]のみ使用する
const uint32_t MSXCTRL_MEM_RD	= 0x09000000;		// 
const uint32_t MSXCTRL_MEM_WR	= 0x0A000000;
const uint32_t MSXCTRL_IO_WR	= 0x06000000;
#else
const uint32_t GP_PICO_LED					= 25;	// Picossci 2B RP2350B: LED Green
const uint32_t GP_PICO_LED2					= 24;	// Picossci 2B RP2350B: LED Red
//
const uint32_t GP_SIRW						= 26;
const uint32_t GP_SLTSL						= 27;
const uint32_t GP_IOREQ						= 28;
const uint32_t GP_WR						= 29;
const uint32_t GP_RD						= 30;
const uint32_t GP_D0_DIR					= 31;
const uint32_t GP_SLIO						= 32;
//
const uint32_t GP_WAIT						= 37;
const uint32_t GP_EXT_OUT_PIN2				= 44;	// J2
const uint32_t GP_EXT_OUT_PIN				= 45;	// J1
const uint32_t GP_MODE_SW					= 47;	// pulluped, H=Cartridge mode, L=Reader Mode
//
// [x][/RD][/WR][/IORQ], [/SLTSL][/SIRW][x][x], [A15-A8], [A7-A0], [D7-D0]
const uint32_t MSXCTRL_MASK		= 0x78000000;		// [/RD][/WR][/IORQ][/SLTSL] のみを使用する
const uint32_t MSXCTRL_MEM_RD	= 0x48000000;
const uint32_t MSXCTRL_MEM_WR	= 0x28000000;
const uint32_t MSXCTRL_IO_WR	= 0x30000000;
#endif


// SPI
#define SPIDEV spi0
#define MMC_SPI_CSN_PIN 	33		// spi0 CSn
#define MMC_SPI_SCK_PIN 	34		// spi0 SCK
#define MMC_SPI_MOSI_PIN  	35	 	// spi0 MOSI
#define MMC_SPI_MISO_PIN  	36		// spi0 MISO

// LED (Picossci 2B RP2350B)
#define	LED_ON	(1)
#define	LED_OFF	(0)

