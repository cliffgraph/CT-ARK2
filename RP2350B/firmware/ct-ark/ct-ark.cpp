/**
 * CT-ARK2 (RP2350B firmware)
 * Copyright (c) 2023-2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/


//#define FOR_DEBUG_PRINT

#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include <hardware/clocks.h>	 // set_sys_clock_khz()
#include "hardware/spi.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include <hardware/flash.h>
#include "ff.h"
#include "diskio.h"
#include "ct-ark.pio.h"
#include "def_gpio.h"
#include "global.h"
#include "sdfat.h"
#include "tools.h"
#include "if_msxpico.h"
#include "epd2in9g.h"
#include "tool_bitmap.h"

//const int BMPFSIZE = 4062;	// bitmap(for e-Paser) file size
const int BMPFSIZE = 113718;	// bitmap(for e-Paser) file size

struct INITGPTABLE {
	int gpno;
	int direction;
	bool bPullup;
	int	init_value;
};

// カートリッジ動作モード
static const INITGPTABLE g_CartridgeMode_GpioTable[] = {
	{ GP_D0_DIR,		GPIO_OUT,		false, 0, },	// 0=MSX->PICO	(必ずプルアップはfalseとする）
//
	{ GP_MODE_SW,		GPIO_IN,		true,  0, },	// (プルアップは必ずtrueとする）
	{ GP_EXT_OUT_PIN,	GPIO_OUT,		false, 0, },
	{ GP_EXT_OUT_PIN2,	GPIO_OUT,		false, 0, },

	{ GP_WAIT,			GPIO_OUT,		false, 1, },
#ifdef WAVESHARE_CORE2350B
 	{ GP_PICO_LED,		GPIO_OUT,		false, LED_OFF, },
#else
	{ GP_PICO_LED,		GPIO_IN,		false, 0, },
	{ GP_PICO_LED2,		GPIO_IN,		false, 0, },
#endif
//
	{ GP_D0,			GPIO_IN,		false, 0, },
	{ GP_D1,			GPIO_IN,		false, 0, },
	{ GP_D2,			GPIO_IN,		false, 0, },
	{ GP_D3,			GPIO_IN,		false, 0, },
	{ GP_D4,			GPIO_IN,		false, 0, },
	{ GP_D5,			GPIO_IN,		false, 0, },
	{ GP_D6,			GPIO_IN,		false, 0, },
	{ GP_D7,			GPIO_IN,		false, 0, },
//
	{ GP_A0,			GPIO_IN,		false, 0, },
	{ GP_A1,			GPIO_IN,		false, 0, },
	{ GP_A2,			GPIO_IN,		false, 0, },
	{ GP_A3,			GPIO_IN,		false, 0, },
	{ GP_A4,			GPIO_IN,		false, 0, },
	{ GP_A5,			GPIO_IN,		false, 0, },
	{ GP_A6,			GPIO_IN,		false, 0, },
	{ GP_A7,			GPIO_IN,		false, 0, },
	{ GP_A8,			GPIO_IN,		false, 0, },
	{ GP_A9,			GPIO_IN,		false, 0, },
	{ GP_A10,			GPIO_IN,		false, 0, },
	{ GP_A11,			GPIO_IN,		false, 0, },
	{ GP_A12,			GPIO_IN,		false, 0, },
	{ GP_A13,			GPIO_IN,		false, 0, },
	{ GP_A14,			GPIO_IN,		false, 0, },
	{ GP_A15,			GPIO_IN,		false, 0, },
//
	{ GP_SIRW,			GPIO_IN,		false, 0, },
	{ GP_RD,			GPIO_IN,		false, 0, },
	{ GP_WR,			GPIO_IN,		false, 0, },
	{ GP_SLTSL,			GPIO_IN,		false, 0, },
	{ GP_IOREQ,			GPIO_IN,		false, 0, },
	{ GP_SLIO,			GPIO_IN,		false, 0, },
	{ -1,				0,				false, 0, },	// eot
};


//uint8_t *g_pFlashRom = (uint8_t *)(XIP_BASE + TOPADDR_FLASHROM_IMAGE);
//uint8_t *g_pFlashRom = (uint8_t *)(XIP_NOCACHE_BASE + TOPADDR_FLASHROM_IMAGE);
//uint8_t *g_pFlashRom = (uint8_t *)(XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE + TOPADDR_FLASHROM_IMAGE);
uint8_t *g_pFlashRom = (uint8_t *)(XIP_NOCACHE_NOALLOC_BASE + TOPADDR_FLASHROM_IMAGE);

static void clearRomName(char *p)
{
	//strcpy(p, "...         ");		// 12-length
	p[0] = '\0';
	return;
}

//static const int MAX_FILERECORD = 254;
static const int MAX_FILERECORD = 999;
struct SDCARDINFO
{
	uint8_t					Status;
	int						NumFiles;
	MSXPICOIF_FILERECORD	Files[MAX_FILERECORD];
};
static SDCARDINFO	g_SdCard;
static Epd2in9g 	g_EPaper;

static const char *pSDWORKDIR = "\\ct-ark2";
static const char *pCONFIGDIR = "\\ct-ark2c";
static char tempWorkPath[255+1];
static char tempWorkFile[LEN_FILE_NAME+1];
static void makeDirWork()
{
	sd_fatMakdir(pSDWORKDIR);
	return;
}

static bool findConfigFile(const char *pName)
{
	sprintf(tempWorkPath, "%s\\%s", pCONFIGDIR, pName);
	bool b = sd_fatExistFile(tempWorkPath);
	return b;
}

static void updateSdCardInfomation(SDCARDINFO *pSd)
{
	static FATFS g_fs;
    FRESULT ret = f_mount( &g_fs, "", 1 );
    if( ret != FR_OK ) {
		// no sd card
		pSd->NumFiles = 0;
		pSd->Status = 0x00;
        return;
    }
	DIR dirObj;
	FILINFO fno;
	FRESULT fr;
	int count = 0;

	fr = f_findfirst(&dirObj, &fno, pSDWORKDIR, "*.ROM");
    while (fr == FR_OK && fno.fname[0] && count < MAX_FILERECORD) {
		auto &item = pSd->Files[count++];
		item.no = count;
		strcpy((char*)item.name, fno.fname);
		item.fsize = (fno.fsize/1024) + (((fno.fsize%1024)!=0)?1:0);
        fr = f_findnext(&dirObj, &fno); 
    }
	f_closedir(&dirObj);

	fr = f_findfirst(&dirObj, &fno, pSDWORKDIR, "*.BAS");
    while (fr == FR_OK && fno.fname[0] && count < MAX_FILERECORD) {
		auto &item = pSd->Files[count++];
		item.no = count;
		strcpy((char*)item.name, fno.fname);
		item.fsize = (fno.fsize/1024) + (((fno.fsize%1024)!=0)?1:0);
        fr = f_findnext(&dirObj, &fno);
    }
	f_closedir(&dirObj);
	//
	pSd->NumFiles = count;
	pSd->Status = 0x01;
	return;
}

static void setupIfReqFileList(const int reqTopFileNo, const SDCARDINFO &sd, STR_REQ_FILELIST *pDest)
{
	int topIndex = 0;
	int sdNumFiles;

	// ファイルが0個、もしくは要求先頭番号が正規の値ではない場合は、ファイルの情報を作成しないようにする
	if( (sd.Status == 0) || (reqTopFileNo <= 0 && MAX_FILERECORD < reqTopFileNo) ) {
		pDest->status = 0;
		topIndex = 0;
		sdNumFiles = 0;
	}
	else {
		pDest->status = sd.Status;
		topIndex = reqTopFileNo -1;
		sdNumFiles = sd.NumFiles;
	}

	// ファイル名の格納
	int cnt = 0;
	for(; cnt < PAGE_FILERECORDS; ++cnt) {
		const int index = topIndex + cnt;
		if( sdNumFiles <= index )
			break;
		pDest->files[cnt] = sd.Files[index];
	}
	//
	for(; cnt < PAGE_FILERECORDS; ++cnt) {
		auto &item = pDest->files[cnt];
		item.no = 0;
		item.name[0] = '.';
		item.name[1] = '.';
		item.name[2] = '.';
		item.name[3] = '\0';
		item.fsize = 0;
	}
	return;
}

static void setupGpio(const INITGPTABLE pTable[] )
{
	for (int t = 0; pTable[t].gpno != -1; ++t) {
		const int no = pTable[t].gpno;
		gpio_init(no);
		//gpio_set_irq_enabled(no, 0xf, false);
		gpio_put(no, pTable[t].init_value);			// PIN方向を決める前に値をセットする
		gpio_set_dir(no, pTable[t].direction);
		if (pTable[t].bPullup)
		 	gpio_pull_up(no);
		else
		 	gpio_disable_pulls(no);
	}
	return;
}

static void stopZ80()
{
	gpio_put(GP_WAIT, 1);
}

static void goZ80()
{
	// /WAIT信号を非アクティブにして、MSX(Z80）の動作を開始させる
	gpio_put(GP_WAIT, 0); // 0=WAIT-OFF(/WAIT信号=OPEN)、1=WAIT-ON(/WAIT信号=L)
}

static const char *g_pPATH_BOOTNAME = "bootname";
static bool saveBootNameFile(const char *pNameArea, const size_t sz)
{
	makeDirWork();
	sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, g_pPATH_BOOTNAME);
	sd_fatWriteFileTo(tempWorkPath, pNameArea, (int)sz );
	return true;
}

static bool loadBootNameFile(const char *pNameArea)
{
	bool bGood = false;
	UINT readSize = 0;
	static const UINT SIZEF = LEN_FILE_NAME+1;
	sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, g_pPATH_BOOTNAME);
	if(sd_fatReadFileFrom(tempWorkPath, SIZEF, (uint8_t*)pNameArea, &readSize) ) {
		bGood = (0 <readSize && readSize <= SIZEF) ? true : false;
	}
	return bGood;
}

static bool loadSysProgram(const char *pFname)
{
	bool bGood = false;
	UINT readSize = 0;
	sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, pFname);

printf("> %s\n", tempWorkPath);

	auto *p = g_WorkRam.GetPtrPage(0, 1);
	if(sd_fatReadFileFrom(tempWorkPath, MEM_16K_SIZE, p, &readSize) ) {
		if( readSize <= MEM_16K_SIZE ){
			bGood = true;
		}
	}
	return bGood;
}

static void taskType_MAPPERRAM(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD)
{
 	for( int t = 0; t < MSX_PAGE_NUM; t++){
		const int blockNo = MSX_PAGE_NUM-t-1;
		g_WorkRam.SetMemMap(0, t, blockNo);
 	}
 	for( int t = 0; t < MSX_PAGE_NUM; t++){
		g_WorkRam.Blocks[t].mem[0x20] = 'S';
		g_WorkRam.Blocks[t].mem[0x21] = '0'+t;
 	}

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;

		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const int pageNo = msxAddr >> 14;
			auto *pMem = g_WorkRam.GetPtrPage(0, pageNo);
			if( pMem != nullptr ){
				const uint16_t offset = msxAddr & 0x3fff;
				const uint32_t dt32 = pMem[offset];
				pio_sm_put(pio, smCartridge, dt32);
			}
			else {
				pio_sm_put(pio, smCartridge, 0xff);
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const int pageNo = msxAddr >> 14;
			auto *pMem = g_WorkRam.GetPtrPage(0, pageNo);
			if( pMem != nullptr ){
				const uint8_t dt8 = wgBus & 0xff;
				const uint16_t offset = msxAddr & 0x3fff;
				pMem[offset] = dt8;
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		//IOREQ & WR（I/Oライト）
		else if( msxCtrl == MSXCTRL_IO_WR ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint8_t dt8 = wgBus & 0xff;
			const int pg = (int)((wgBus>>8) & 0xff) - 0xfc;
			if( 0 <= pg ){
				// メモリーマッパーに関する
				g_WorkRam.SetMemMap(0, pg, dt8);
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_ASCII16K_MRAM(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD,
	const uint32_t KNLBLKS)
{
	// 下記は、KNLBLKS = 4 の例
	// 拡張スロットの仕組みを利用して、
	// DOS2カーネルのROMの部分と、メモリマッパーRAMを共存させる仕組みになっています。
	// ExSlot#0は何もなし
	// ExSlot#1にメモリマッパーRAM(128KB)
	// ExSlot#2は、、不明
	// ExSlot#3のPage1にROM（16KB x 4 のバンク切替）
	//
	// CT-ARK(Pico)には、12個(TOTAL_MEM_16K)のブロックRAMが有るので、それを次のように割り当てます
	// Blocks[0-7] = マッパーRAM
	// Blocks[8-1] = ROM(16KB x 4 のバンク切替)


	// 現状、KNLBLKS={4,8}しか受け付けない
	if( KNLBLKS != 4 && KNLBLKS != 8 )
		return;
	const uint8_t BANKMASK = (KNLBLKS==4) ? 0x03 : 0x07;
	const int RAMSLOTNO = 1;

	// メモリマッパーRAM領域の準備
	g_WorkRam.SetNumMapperBlocks(ARKWORKRAM::TOTAL_MEM_16K - KNLBLKS);
	g_WorkRam.SetVoidMem();
 	for( int t = 0; t < MSX_PAGE_NUM; t++){
		const int blockNo = MSX_PAGE_NUM-t-1;
		g_WorkRam.SetMemMap(RAMSLOTNO, t, blockNo);
 	}
 	for( int t = 0; t < MSX_PAGE_NUM; t++){
		auto *p = g_WorkRam.Blocks[t].mem;
		p[0x20] = 'S';
		p[0x21] = '0'+t;
 	}

	// ROM領域の準備
	uint8_t *pRom = g_WorkRam.Blocks[ARKWORKRAM::TOTAL_MEM_16K - KNLBLKS +0].mem;
	memcpy(pRom, g_pFlashRom, MEM_16K_SIZE * KNLBLKS);

	// 拡張スロット選択レジスタ
	uint8_t regExpSlot = 0x00;
	uint8_t	slotNoByPageNo[4] = {0,0,0,0};
	uint8_t *const bank[8] = {
		&pRom[MEM_16K_SIZE*0], &pRom[MEM_16K_SIZE*1], &pRom[MEM_16K_SIZE*2], &pRom[MEM_16K_SIZE*3],
		&pRom[MEM_16K_SIZE*4], &pRom[MEM_16K_SIZE*5], &pRom[MEM_16K_SIZE*6], &pRom[MEM_16K_SIZE*7], };

	g_WorkRam.SetPtrPage(3, 1, bank[0]);

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		// [x][/RD][/WR][/IORQ], [/SLTSL][/SIRW][x][x], [A15-A8],[A7-A0], [D7-D0]
		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;

		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			if( msxAddr == 0xffff ){
				// 拡張スロット選択レジスタの内容を反転したものを返す
				const uint32_t dt32 = regExpSlot ^ 0xff;
				pio_sm_put(pio, smCartridge, dt32);
			}
			else {
				// メモリ
				const int pageNo = msxAddr >> 14;
				const int slotNo = slotNoByPageNo[pageNo];
				auto *pMem = g_WorkRam.GetPtrPage(slotNo, pageNo);
				if( pMem != nullptr ) {
					const uint32_t offset = msxAddr & 0x3fff;
					const uint32_t dt32 = pMem[offset];
					pio_sm_put(pio, smCartridge, dt32);
				}
				else {
					pio_sm_put(pio, smCartridge, 0xff);
				}
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const uint8_t dt8 = wgBus & 0xff;
			if( msxAddr == 0xffff ){
				// 拡張スロット選択レジスタ。そのまま格納する
				// また、各ページごとの配列にスロット番号を分解して格納しておく
				regExpSlot = dt8;
				slotNoByPageNo[0] = (dt8 >> 0) & 0x03;
				slotNoByPageNo[1] = (dt8 >> 2) & 0x03;
				slotNoByPageNo[2] = (dt8 >> 4) & 0x03;
				slotNoByPageNo[3] = (dt8 >> 6) & 0x03;
			}
			else {
				const int pageNo = msxAddr >> 14;
				const int slotNo = slotNoByPageNo[pageNo];
				if( slotNo == RAMSLOTNO ) {
					auto *pMem = g_WorkRam.GetPtrPage(RAMSLOTNO, pageNo);
					if( pMem != nullptr ){
						const uint32_t offset = msxAddr & 0x3fff;
						pMem[offset] = dt8;
					}
				}
				else if( slotNo == 3 && (msxAddr == 0x6000 || msxAddr == 0x7FFE) ) {
					g_WorkRam.SetPtrPage(3, 1, bank[dt8 & BANKMASK]);
				}
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		//IOREQ & WR（I/Oライト）
		else if( msxCtrl == MSXCTRL_IO_WR ) {
			const int pg = (int)((wgBus>>8) & 0xff) - 0xfc;
			const uint8_t dt8 = wgBus & 0xff;
			if( 0 <= pg ){
				// メモリーマッパーに関する
				g_WorkRam.SetMemMap(RAMSLOTNO, pg, dt8);
			}
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_ROM64K(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD,
	const bool bFlashROM)
{
	uint8_t *pMem;
	if( bFlashROM ){
		pMem = g_pFlashRom;
	}
	else{
		pMem = g_WorkRam.GetPtrPage(0, 0);
		memcpy(pMem, g_pFlashRom, MEM_64K_SIZE);
	}

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		// [x][/RD][/WR][/IORQ],[/SLTSL][/SIRW][x][x],[A15-A8],[A7-A0],[D7-D0]
		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;
		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const uint32_t dt32 = pMem[msxAddr];
			pio_sm_put(pio, smCartridge, dt32);
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			// do nothing
		}
		// IOREQ & WR
		else if( msxCtrl == MSXCTRL_IO_WR) {
			// do nothing
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

// CT-ARKメニューモード用
static const char g_pStrMagicWord[] = "OPEN THE ARK";
static const char g_pStrVer[] = "1.00\0";				// version number of "ct-ark menu"	// VERSION
static char g_pStrClkMHz[5+1] = "()";

static void taskType_CTARKMENU(
	PIO pio, uint adCartridge, uint smCartridge,
	uint32_t RETCMD)
{
	// メニューモードではページ2のみ使用する。
	// ただし念のためページ0-3はワークエリアとしては使用しないようにする
	// 残りのRAMは、Core0側で作業用として使用することに注意する
	auto *pMem = g_WorkRam.GetPtrPage(0, 0);

	//
	const uint16_t MSXPICO_IF_Z80ADDR = 0x5F00;
	auto *pIf = reinterpret_cast<volatile MSXPICO_IF *>(&pMem[MSXPICO_IF_Z80ADDR]);
	const char *pStrArk = g_pStrMagicWord;
	uint16_t z80_addrArk = MSXPICO_IF_Z80ADDR;

	pIf->cmd_res	= IFCMD::NONE;
	pIf->cmds.code	= IFCMD::NONE;
	g_bActiveCtArk	= false;

	// CTARKMENUではデフォルトのクロックで動作させる
	const int defClk = g_pCAI->GetDefaultSysClk();
	if( g_pCAI->SysClk != defClk )
		set_sys_clock_khz(defClk * 1000, true);

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;
		const uint16_t msxAddr = (wgBus>>8) & 0xffff;
		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint32_t dt32 = pMem[msxAddr];
			pio_sm_put(pio, smCartridge, dt32);
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint8_t dt8 = wgBus & 0xff;
			if( g_bActiveCtArk ){
				pMem[msxAddr] = dt8;
			}
			// Z80側からのマジックワード書き込みをチェックする
			else if( msxAddr == z80_addrArk && *pStrArk == dt8 ) {
				++pStrArk, ++z80_addrArk;
				if( *pStrArk == '\0' ){
					g_bActiveCtArk = true;
				}
			}
		}
		// IOREQ & WR
		else if( msxCtrl == MSXCTRL_IO_WR) {
			// do nothing
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_ASCII8K(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD,
	const bool bFlashROM)
{
	uint8_t *pMem;
	if( bFlashROM ) {
		// 直接フラッシュROM上で動作させるモード
		pMem = g_pFlashRom;
	}
	else{
		// SRAMにコピーしてから動作させるモード
		pMem = g_WorkRam.Blocks[0].mem;
		memcpy(pMem, g_pFlashRom, g_WorkRam.GetSize());
	}

	const int WINDNUM = 8;
	int banks[WINDNUM] = {0,0,0,0,0,0,0,0};
	uint8_t *pWind[WINDNUM];
	for( int t = 0;  t < WINDNUM; ++t) {
		pWind[t] = &pMem[0x2000*banks[t]];
	}

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;
		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const int wnd = (msxAddr>>13);
			const uint32_t off = msxAddr & 0x1FFF;
			const uint32_t dt32 = (pWind[wnd])[off];
			pio_sm_put(pio, smCartridge, dt32);
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint8_t dt8 = wgBus & 0xff;
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			if( 0x6000 <= msxAddr && msxAddr < 0x8000) {
				const int areaNo = ((msxAddr >> 11) & 0x03) + 2;
				if( banks[areaNo] != dt8 ) {
					banks[areaNo] = dt8;
					pWind[areaNo] = &pMem[0x2000*dt8];
				}
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// IOREQ & WR
		else if( msxCtrl == MSXCTRL_IO_WR) {
			// do nothing
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_ASCII16K(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD,
	const bool bFlashROM)
{
	uint8_t *pMem;
	if( bFlashROM ) {
		// 直接フラッシュROM上で動作させるモード
		pMem = g_pFlashRom;
	}
	else{
		// SRAMにコピーしてから動作させるモード
		pMem = g_WorkRam.Blocks[0].mem;
		memcpy(pMem, g_pFlashRom, g_WorkRam.GetSize());
	}

	const int WINDNUM = 4;
	int banks[WINDNUM] = {0,0,0,0};
	uint8_t *pWind[WINDNUM];
	for( int t = 0;  t < WINDNUM; ++t) {
		pWind[t] = &pMem[0x4000*banks[t]];
	}

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;
		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const int wnd = (msxAddr>>14);
			const uint32_t off = msxAddr & 0x3FFF;
			const uint32_t dt32 = (pWind[wnd])[off];
			pio_sm_put(pio, smCartridge, dt32);
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint8_t dt8 = wgBus & 0xff;
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			if( 0x6000 <= msxAddr && msxAddr < 0x8000) {
				const int areaNo = ((msxAddr >> 12) & 0x01) + 1;
				if( banks[areaNo] != dt8 ) {
					banks[areaNo] = dt8;
					pWind[areaNo] = &pMem[0x4000*dt8];
				}
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// IOREQ & WR
		else if( msxCtrl == MSXCTRL_IO_WR) {
			// do nothing
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_RTYPE(
	PIO pio, uint adCartridge, uint smCartridge, uint32_t RETCMD)
{
	uint8_t *pMem;
	// SRAMにコピーしてから動作させる
	pMem = g_WorkRam.Blocks[0].mem;
	memcpy(pMem, g_pFlashRom, g_WorkRam.GetSize());

	const int WINDNUM = 4;
	int banks[WINDNUM] = {0,0x0f,0,0};
	uint8_t *pWind[WINDNUM];
	for( int t = 0;  t < WINDNUM; ++t) {
		pWind[t] = &pMem[0x4000*banks[t]];
	}

	goZ80();

	for(;;){
		gpio_put(GP_EXT_OUT_PIN, 1);	// Check

		const uint32_t wgBus = pio_sm_get_blocking(pio, smCartridge);
		const uint32_t msxCtrl = (~wgBus) & MSXCTRL_MASK;
		// SLSTS & RD（メモリリード）
		if( msxCtrl == MSXCTRL_MEM_RD ) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			const int wnd = (msxAddr>>14);
			const uint32_t off = msxAddr & 0x3FFF;
			const uint32_t dt32 = (pWind[wnd])[off];
			pio_sm_put(pio, smCartridge, dt32);
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// SLTSL & WR（メモリライト）
		else if( msxCtrl == MSXCTRL_MEM_WR) {
			gpio_put(GP_EXT_OUT_PIN, 0);	// Check
			const uint8_t dt8 = wgBus & 0xff;
			const uint16_t msxAddr = (wgBus>>8) & 0xffff;
			if( 0x7000 <= msxAddr && msxAddr < 0x77ff) {
				if( banks[2] != dt8 ) {
					banks[2] = dt8;
					pWind[2] = &pMem[0x4000*dt8];
				}
			}
			gpio_put(GP_EXT_OUT_PIN, 1);	// Check
		}
		// IOREQ & WR
		else if( msxCtrl == MSXCTRL_IO_WR) {
			// do nothing
		}
		else {
			pio_sm_exec(pio, smCartridge, RETCMD);
		}
	}
	return;
}

static void taskType_FAIL()
{
	goZ80();

//	uint8_t b = 0;
	for(;;) {
//		gpio_put(GP_PICO_LED, b^=1);
		sleep_ms(300);
	}
	return;
}

static void Core1Task()
{
	const uint16_t RETCMD = 0x0000;	// jmp 0 with side-set 0 and no delay

	// PIO の開始
 	PIO pio = pio0;
	uint ad;
	uint sm;

#ifdef WAVESHARE_CORE2350B
	ad	= pio_add_program( pio, &pio_cartridge_waveshare_program );
	sm = pio_claim_unused_sm( pio, true );
	pio_cartridge_waveshare_program_init(
		pio, sm, ad, GP_D0, 31, GP_RD, GP_D0_DIR, 1);
#else
	ad	= pio_add_program( pio, &pio_cartridge_program );
	sm = pio_claim_unused_sm( pio, true );
	pio_cartridge_program_init(
		pio, sm, ad, GP_D0, 31, GP_RD, GP_D0_DIR, 1);
#endif

		// メモリアクセス・メインループ
	switch(g_RomType)
	{
		case SETROMTYPE::MAPPERERAM:
			taskType_MAPPERRAM(pio, ad, sm, RETCMD);
			break;
		case SETROMTYPE::MSXDOS2_320KB:
			taskType_ASCII16K_MRAM(pio, ad, sm, RETCMD, 4/*ASCII16K ROM部のバンク数*/);
			break;
		case SETROMTYPE::NEXTOR128KB_256KB:
			taskType_ASCII16K_MRAM(pio, ad, sm, RETCMD, 8/*ASCII16K ROM部のバンク数*/);
			break;
		case SETROMTYPE::ROM_PAGE_0:
		case SETROMTYPE::ROM_PAGE_1:
		case SETROMTYPE::ROM_PAGE_2:
		case SETROMTYPE::ROM_PAGE_3:
			taskType_ROM64K(pio, ad, sm, RETCMD, false/*RAM*/);
			break;
		case SETROMTYPE::ASC8K_RAM:
			taskType_ASCII8K(pio, ad, sm, RETCMD, false/*RAM*/);
			break;
		case SETROMTYPE::ASC8K_FLASHROM:
		{
			// サイズでRAM、FlashROM動作を切り替える
			// FlashROM動作の場合、300MHzで動作するようにクロックアップする
			const bool bFlash = (g_WorkRam.GetSize() <= (int)(g_Num8KBlosks*8*1024)) ? true : false;
			if( bFlash && g_pCAI->SysClk < 300 ){
				set_sys_clock_khz(300 * 1000, true);
			}
			taskType_ASCII8K(pio, ad, sm, RETCMD, bFlash);
			break;
		}
		case SETROMTYPE::ASC16K_RAM:
			taskType_ASCII16K(pio, ad, sm, RETCMD, false/*RAM*/);
			break;
		case SETROMTYPE::ASC16K_FLASHROM:
		{
			const bool bFlash = (g_WorkRam.GetSize() <= (int)(g_Num8KBlosks*8*1024)) ? true : false;
			if( bFlash && g_pCAI->SysClk < 300 ){
				set_sys_clock_khz(300 * 1000, true);
			}
			taskType_ASCII16K(pio, ad, sm, RETCMD, bFlash);
			break;
		}
		case SETROMTYPE::RTYPE:
		{
			taskType_RTYPE(pio, ad, sm, RETCMD);
			break;
		}
		case SETROMTYPE::CTARKMENU:
			taskType_CTARKMENU(pio, ad, sm, RETCMD);
			break;
		case SETROMTYPE::NONE:
		default:
			taskType_FAIL();
			break;
	}
	return;
}


/* PICO搭載のFlashROMの消去を行う（4KBytes単位で消去される）
@param destAddr 書き込み先のFlashROMアドレス。4Kbytesにアライメントされていること。
@param sz データサイズ(必要な消去ブロック数は内部で算出するので、ここはbytes単位指定でOK、)
*/
static void eraseFlashROM(const uint32_t destAddr, size_t sz)
{
	static const int ERSZ = 4*1024;
	const size_t eraseSz = ((sz/(ERSZ)) + (((sz%ERSZ)==0)?0:1)) * ERSZ;
	const uint32_t ints = save_and_disable_interrupts();
	flash_range_erase(destAddr, eraseSz);
	restore_interrupts(ints);
	return;
}

/* PICO搭載のFlashROMに書き込みを行う
@param destAddr 書き込み先のFlashROMアドレス。256bytesにアライメントされていること。
@param pSrc 書込元データへのポインタ。書込元データへの領域は256bytesサイズ単位で参照しても問題ないこと
@param sz データサイズ(必要なセグメント数は内部で算出するので、ここはbytes単位指定でOK、)
@param bErase 消去も行う(destAddrが4KBytesにアライメントされていること。また4KBytes単位で消去される）
*/
static void writeToFlashROM(const uint32_t destAddr, const uint8_t *pSrc, size_t sz)
{
	static const int WRSZ = 256;
	const size_t writeSz = ((sz/(WRSZ)) + (((sz%WRSZ)==0)?0:1)) * WRSZ;
	const uint32_t ints = save_and_disable_interrupts();
	flash_range_program(destAddr, pSrc, writeSz);
	restore_interrupts(ints);
	return;
}


/** FR_CTARK_INFO をFlashROMに書き込む
*/
static void writeBootRomInfoToFlashROM(const FR_CTARK_INFO *p)
{
	eraseFlashROM(TOPADDR_CTARK_INFO, sizeof(FR_CTARK_INFO));
	writeToFlashROM(TOPADDR_CTARK_INFO, (const uint8_t*)p, sizeof(FR_CTARK_INFO));
	return;
}

static bool writeFileImageToFlashROM(
	const SETROMTYPE romType, const uint32_t fileSize, const char *pPath)
{
	if( fileSize == 0 )
		return false;
	bool bOk = true;

	// RomTypeと書きこみ範囲から消去範囲を確定する
	uint32_t writeAddr, writeSize;
	uint32_t eraseAddr, eraseSize;
	switch(romType)
	{
		// ROM_PAGE_0～3 いずれも先頭から64KBをeraseする
		case SETROMTYPE::ROM_PAGE_0:
		{
			const uint32_t AREASIZE = MEM_16K_SIZE * 4;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			writeSize = (fileSize <= AREASIZE) ? fileSize : AREASIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = MEM_64K_SIZE;
			break;
		}
		case SETROMTYPE::ROM_PAGE_1:
		{
			const uint32_t AREASIZE = MEM_16K_SIZE * 3;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 1;
			writeSize = (fileSize <= AREASIZE) ? fileSize : AREASIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = MEM_64K_SIZE;
			break;
		}
		case SETROMTYPE::ROM_PAGE_2:
		{
			const uint32_t AREASIZE = MEM_16K_SIZE * 2;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 2;
			writeSize = (fileSize <= AREASIZE) ? fileSize : AREASIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = MEM_64K_SIZE;
			break;
		}
		case SETROMTYPE::ROM_PAGE_3:
		{
			const uint32_t AREASIZE = MEM_16K_SIZE * 1;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 3;
			writeSize = (fileSize <= AREASIZE) ? fileSize : AREASIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = MEM_64K_SIZE;
			break;
		}
		case SETROMTYPE::MSXDOS2_320KB:
		{
			const uint32_t ENVSIZE = MEM_64K_SIZE*1; /* 64KB */
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			writeSize = (fileSize <= ENVSIZE) ? fileSize : ENVSIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = writeSize;
			break;
		}
		case SETROMTYPE::NEXTOR128KB_256KB:
		{
			const uint32_t ENVSIZE = MEM_64K_SIZE*2; /* 128KB */
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			writeSize = (fileSize <= ENVSIZE) ? fileSize : ENVSIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = writeSize;
			break;
		}
		case SETROMTYPE::ASC8K_RAM:
		case SETROMTYPE::ASC16K_RAM:
		case SETROMTYPE::RTYPE:
		{
			const uint32_t ENVSIZE = ARKWORKRAM::TOTAL_MEM_16K * MEM_16K_SIZE;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			writeSize = (fileSize <= ENVSIZE) ? fileSize : ENVSIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = writeSize;
			break;
		}
		case SETROMTYPE::ASC8K_FLASHROM:
		case SETROMTYPE::ASC16K_FLASHROM:
		{
			const uint32_t ENVSIZE = SIZE_FLASHROM_IMAGE;
			writeAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			writeSize = (fileSize <= ENVSIZE) ? fileSize : ENVSIZE;
			eraseAddr = TOPADDR_FLASHROM_IMAGE + MEM_16K_SIZE * 0;
			eraseSize = writeSize;
			break;
		}
		default:
		{
			writeSize = 0;
			eraseSize = 0;
			break;
		}
	}

	// 消去の実行
	if( 0 < eraseSize ){
		eraseFlashROM(eraseAddr, eraseSize);
	}

	// ファイルの読み込みとFlashROMへの書き込みは BLOCK8K 単位で行う。
	const int BLOCK8K = 8*1024;
	const int numBlock = (writeSize/BLOCK8K) + (((writeSize%BLOCK8K)==0)?0:1);
	// ファイルを一時的に読み込んでおく（一時領域へのポインタを得る）
	uint8_t *pDt = g_WorkRam.Blocks[MSX_PAGE_NUM].mem;
	// 
	uint32_t writePtr = writeAddr;
	for( int t = 0; t < numBlock; ++t) {
		UINT readSize;
		if( !sd_fatReadFileFromOffset(pPath, BLOCK8K*t, BLOCK8K, pDt, &readSize) ) {
			bOk = false;
			break;
		}
		writeToFlashROM(writePtr, pDt, readSize);
		writePtr += readSize;
	}
	return bOk;
}


static void cmd_UPDATE_FILELIST(STR_UPDATE_FILELIST *pIf)
{
	auto &sd = g_SdCard;
	updateSdCardInfomation(&sd);
	pIf->status = sd.Status;
	if( pIf->status == 0 ) {
		pIf->total_num_files = 0;
	}
	else{
		pIf->total_num_files = static_cast<uint16_t>(sd.NumFiles);
	}
	return;
}

static void cmd_REQ_FILELIST(STR_REQ_FILELIST *pIf)
{
	setupIfReqFileList(pIf->reqTopNo, g_SdCard, pIf);
	return;
}

static void cmd_SET_ROM(const STR_SET_ROM &dt)
{
	const int no = dt.file_no;
	auto *pBRI = &g_pCAI->BootRomInfo;
	if( no == 0 ) {
		pBRI->Clear();
		writeBootRomInfoToFlashROM(g_pCAI);
	}
	else if( g_SdCard.Status != 0x00 && no <= g_SdCard.NumFiles ) {
		// Name
		const char *pName = (char*)g_SdCard.Files[no-1].name;
		strcpy((char*)pBRI->FileName, pName);
		// ROM type
		pBRI->RomType = static_cast<SETROMTYPE>(dt.boot_rom_type);
		// FileSize
		sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, pName);
		pBRI->FileSize = sd_fatGetFileSize(tempWorkPath);
		// Block Number
		pBRI->Num8KBlosks = (pBRI->FileSize / (8*1024)) + (((pBRI->FileSize % (8*1024))==0)?0:1);
		// Infomation -> write
		writeBootRomInfoToFlashROM(g_pCAI);
		// ROM image file を Pico内のFlashROMへコピーする
		writeFileImageToFlashROM(pBRI->RomType, pBRI->FileSize, tempWorkPath);
	}
	else {
		// do nothing
	}
	return;
}

static void cmd_DEL_ROMFILE(const STR_DEL_ROMFILE &dt)
{
	const int no = dt.file_no;
	auto *pBRI = &g_pCAI->BootRomInfo;
	if( g_SdCard.Status != 0x00 && 0 < no && no <= g_SdCard.NumFiles ) {
		const char *pName = (char*)g_SdCard.Files[no-1].name;
		if( strcmp(pName, (char*)pBRI->FileName) == 0 ) {
			pBRI->Clear();
			writeBootRomInfoToFlashROM(g_pCAI);
		}
		sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, pName);
		sd_fatRemoveFile(tempWorkPath);
	}
	return;
}

static void cmd_WR_FILE(const STR_WR_FILE &dt)
{
	int cnt = 0;
	char pName[LEN_FILE_SPEC+1];
	for(int t = 0; t < LEN_FILE_SPEC; ++t) {
		uint8_t ch = dt.cmd_fwr_fspec[t];
		if( ch == ' ' || ch == '\x0' )
			break;
		if( t_CheckFilespecChar(ch) ) {
			pName[cnt++] = ch;
		}
	}
	// null-terminator
	pName[cnt] = '\0';

	if( 0 < cnt ) {
		makeDirWork();
		sprintf(tempWorkFile, "%s.ROM", pName);
		sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, tempWorkFile);
		const int blockNo = dt.block_no;
		const auto *pBuff = (char *)&dt.block_data;
		const bool bAppend = (blockNo==0)?false:true;
		sd_fatWriteFileTo(tempWorkPath, pBuff, WR_FILE_BLOSK_SIZE, bAppend );

		// 指定された保存ファイル名が、すでに起動ROMとしてして指定されていたファイル名と同じだった場合、
		// 起動ROMファイル名情報をFlashROMから消去する（起動ROM情報を消去する）。
		// これは同名のROMファイルが既にFlashROMに書き込まれている状態で
		// ROMファイルを作成すると、FlashROMに書き込まれているFlashROMと
		// 作成したROMファイルが同じものだとさあっ隠してしまうのを防ぐため。
		// （すでにFlashROMに書き込まれているファイル名はしてできないようにガードする、でもよかったかも）
		auto *pBRI = &g_pCAI->BootRomInfo;
		if( strcmp(tempWorkFile, (char*)pBRI->FileName) == 0 ) {
			pBRI->Clear();
			writeBootRomInfoToFlashROM(g_pCAI);
		}
	}
	return;
}

static void cmd_BAS2ROM(const STR_BAS2ROM &dt)
{
	int cnt = 0;
	char pName[LEN_FILE_SPEC+1];
	for(int t = 0; t < LEN_FILE_SPEC; ++t) {
		const uint8_t ch = dt.cmd_fwr_fspec[t];
		if( ch == ' ' || ch == '\x0' )
			break;
		if( t_CheckFilespecChar(ch) ) {
			pName[cnt++] = ch;
		}
	}
	pName[cnt] = '\0';
	if( 0 < cnt ) {
		const int no = dt.file_no;
		if( g_SdCard.Status != 0x00 && 0 < no && no <= g_SdCard.NumFiles ) {
			const char *pBasFileName = (char*)g_SdCard.Files[no-1].name;
			sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, pBasFileName);
			volatile uint8_t *pBas = g_WorkRam.Blocks[MSX_PAGE_NUM].mem; // 一時領域
			const int sizeBas = 16*1024*1;
			UINT readSize;
			if( sd_fatReadFileFrom(tempWorkPath, sizeBas, (uint8_t*)pBas, &readSize) ) {
				if( t_ReassignmentAddress(pBas) ) {
					static const uint8_t head[] = {
						'A', 'B',
						0x00,0x00, 0x00,0x00, 0x00,0x00,	// INIT, STATEMENT, DEVICE
						0x10,0x80,	// TEXT
						0x00,0x00,0x00,0x00,0x00,0x00,
					};
					sprintf(tempWorkPath, "%s\\%s.ROM", pSDWORKDIR, pName);
					makeDirWork();
					sd_fatWriteFileTo(tempWorkPath, (const char*)head, sizeof(head), false/*create file*/);
					sd_fatWriteFileTo(tempWorkPath, (const char*)pBas, readSize, true/*append*/);
				}
			}
		}
	}
	return;
}

static void cmd_STORE_ROM(STR_STORE_ROM *pIf)
{
	const int no = pIf->file_no;
	if( g_SdCard.Status == 0x00 || no <= 0 || g_SdCard.NumFiles < no) {
		pIf->err_no = 1;
		return;
	}
	uint8_t *const pWorkBuff = g_WorkRam.Blocks[MSX_PAGE_NUM].mem;
	pIf->err_no = 0/*no error*/;
	const auto reqNo = pIf->block_no;
	if( reqNo == 0 ) {
		uint8_t num_blocks = 0;
		// 選択されたROMファイル番号から、ファイル名を得る
		const char *pFileName = (char*)g_SdCard.Files[no-1].name;
		sprintf(tempWorkPath, "%s\\%s", pSDWORKDIR, pFileName);
		// ファイル名から、ファイルサイズを得る
		uint32_t fsize = sd_fatGetFileSize(tempWorkPath);
		if( fsize == 0 ) {
			pIf->err_no = 1/*error*/;
		}
		else {
			if( MEM_64K_SIZE < fsize ) {
				fsize = MEM_64K_SIZE;
			}
			// ファイルサイズからブロック数を得る
			const uint32_t n = (fsize / STORE_ROM_BLOSK_SIZE) + (((fsize % STORE_ROM_BLOSK_SIZE)==0)?0:1);
			num_blocks = static_cast<uint8_t>(n);
			// ファイルの読み込み
			UINT readSize;
			if( !sd_fatReadFileFrom(tempWorkPath, fsize, pWorkBuff, &readSize) ) {
				pIf->err_no = 1/*error*/;
			}
			else{
				// do nothing
			}
		}
		pIf->str_num_total_block = num_blocks;
	}

	if( reqNo < pIf->str_num_total_block ){
		pIf->str_block = 1;	// 要求ブロックあり
		const uint16_t addr = STORE_ROM_BLOSK_SIZE * reqNo;
		memcpy(pIf->str_block_data, &pWorkBuff[addr], STORE_ROM_BLOSK_SIZE);
	}
	else {
		pIf->str_block = 0;	// 要求ブロック無し
	}
	return;
}

static void cmd_SET_E_PAPER(const int fileNo)
{
	if( fileNo == 0 ) {
		// E-Paperを白色で塗りつぶす
		g_EPaper.Init();
		g_EPaper.ClearScreen(EPD_2IN9G_WHITE);
		g_EPaper.EnterSleep();
		g_EPaper.KeepReset();
	}
	else {
		const int no = fileNo;
		if( g_SdCard.Status != 0x00 && 0 < no && no <= g_SdCard.NumFiles ) {
			const char *pName = (char*)g_SdCard.Files[no-1].name;
			STRFILENAME sp;
			t_SplitPath(pName, &sp);
			sprintf(tempWorkPath, "%s\\%s.BMP", pSDWORKDIR, sp.spec);
			if( sd_fatGetFileSize(tempWorkPath) != BMPFSIZE ) {
				sprintf(tempWorkPath, "%s\\ctark.bmp", pSDWORKDIR);
				if( sd_fatGetFileSize(tempWorkPath) != BMPFSIZE ) {
					return;
				}
			}
			const int workSize = g_WorkRam.GetSize() - (sizeof(MEM_16K)*MSX_PAGE_NUM);
			uint8_t *pWork = g_WorkRam.Blocks[MSX_PAGE_NUM].mem;
			uint8_t *p = t_LoadBmp24ToEPDformat128x296(tempWorkPath, pWork, workSize);
			if( p != nullptr ){
				g_EPaper.Init();
				g_EPaper.DisplayImage(pWork);
				g_EPaper.EnterSleep();
				g_EPaper.KeepReset();
			}
		}
		else {
			// do nothing
		}
	}
	return;
}

static void Core0Task()
{
	bool bMarkedOld = g_bActiveCtArk;

	if( g_RomType != SETROMTYPE::CTARKMENU ) {
		// core0 は停止します
		multicore_lockout_victim_init();
	}
	else {
		auto *pMem = g_WorkRam.GetPtrPage(0, 0);
		auto *pIf = reinterpret_cast<MSXPICO_IF *>(&pMem[0x5F00]);
		auto *pBRI = &g_pCAI->BootRomInfo;
		for(;;) {
			if( bMarkedOld != g_bActiveCtArk ){
				bMarkedOld = g_bActiveCtArk;
				memcpy((void *)pIf->magic_word, g_pStrMagicWord, sizeof(g_pStrMagicWord)-1);
			}
			if( !g_bActiveCtArk || pIf->cmd_res != IFCMD::NONE )
				continue;
			const auto cmd = pIf->cmds.code;
			switch( cmd )
			{
				case IFCMD::UPDATE_INFO:
				{
					sprintf(g_pStrClkMHz, "(%d)", (int)g_pCAI->SysClk);
					memcpy((void *)pIf->pico_side_version, g_pStrVer, sizeof(g_pStrVer));
					memcpy((void *)pIf->pico_clock_mhz, g_pStrClkMHz, sizeof(g_pStrClkMHz));
					strcpy((char*)pIf->boot_rom_name, (char*)pBRI->FileName);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::UPDATE_FILELIST:
				{
					cmd_UPDATE_FILELIST(&pIf->cmds.update_filelist);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::REQ_FILELIST:
				{
					cmd_REQ_FILELIST(&pIf->cmds.req_filelist);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::SET_ROM:
				{
					cmd_SET_ROM(pIf->cmds.set_rom);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::DEL_ROMFILE:
				{
					cmd_DEL_ROMFILE(pIf->cmds.del_romfile);
					strcpy((char*)pIf->boot_rom_name, (char*)pBRI->FileName);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::WR_FILE:
				{
					cmd_WR_FILE(pIf->cmds.wr_file);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::BAS2ROM:	// BASIC
				{
					cmd_BAS2ROM(pIf->cmds.bas2rom);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::STORE_ROM:
				{
					cmd_STORE_ROM(&pIf->cmds.store_rom);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::SET_FREQ:
				{
					const uint16_t clk = pIf->cmds.set_freq.freq_mhz;
					g_pCAI->SysClk = clk + 125;
					// ここでは動作クロックを変更しない、FlashROM領域内に保存するのみ
					writeBootRomInfoToFlashROM(g_pCAI);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::CLEAR_E_PAPER:
				{
					cmd_SET_E_PAPER(0);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				case IFCMD::PRINT_E_PAPER:
				{
					cmd_SET_E_PAPER(pIf->cmds.print_epaper.file_no);
					pIf->cmds.code = IFCMD::NONE;
					pIf->cmd_res = cmd;
					break;
				}
				default:
				{
					// do nothing
					break;
				}
			}
		}
	}
	return;
}

static uint32_t timercnt = 0;
bool __time_critical_func(timerproc_fot_ff)(repeating_timer_t *rt)
{
	++timercnt;
	disk_timerproc();
	return true;
}
uint32_t __time_critical_func(GetTimerCounterMS)()
{
	return timercnt;
}


/**
 * エントリー
 */
int main()
{
#ifdef FOR_DEBUG_PRINT
	stdio_init_all();
	sleep_ms(1000);
	printf("CT-ARK2\n");
#endif
	// FlashROMに書き込まれているROM情報をRAM上の領域にコピーする
	*g_pCAI = *(reinterpret_cast<const FR_CTARK_INFO*>(XIP_BASE+TOPADDR_CTARK_INFO));
	if( !g_pCAI->CheckInfo() ) {
		g_pCAI->ResetInfo();
	}

	set_sys_clock_khz(g_pCAI->SysClk * 1000, true);

	setupGpio(g_CartridgeMode_GpioTable);

	// タイマー設定
	static repeating_timer_t tim;
	add_repeating_timer_ms (1/*ms*/, timerproc_fot_ff, nullptr, &tim);

#ifdef FOR_DEBUG_PRINT
	DSTATUS disksts = disk_initialize(0);
	printf("disksts:%d\n", (int)disksts);
#else
	disk_initialize(0);
#endif


	// "ct-ark2c/allreset
	if( findConfigFile("ALLRESET") ) {
		g_pCAI->ResetInfo();
		writeBootRomInfoToFlashROM(g_pCAI);
	}

	g_bActiveCtArk = false;
	g_RomType = SETROMTYPE::MAPPERERAM;
	g_Num8KBlosks = 0;

	// SW［■・］の場合、
	//	FlashROMに書き込まれたROMイメージを実行するモード
	bool bModeSwPosLeft = gpio_get(GP_MODE_SW);
#ifdef FOR_DEBUG_PRINT
	printf("SW:%d\n", (int)bModeSwPosLeft);
#endif
	if (bModeSwPosLeft) {
		auto *pBRI = &g_pCAI->BootRomInfo;
		if( pBRI->FileName[0] != '\0' ) {
			g_RomType = pBRI->RomType;
			g_Num8KBlosks = pBRI->Num8KBlosks;
		}
		else {
			// ROMイメージがなかった場合は、0x8010 番地以降にその旨を書き込んでおく
			auto *p = g_WorkRam.GetPtrPage(0, 1);
			memcpy((void*)&p[0x10], "not found rom image", 19);
		}
	}
	// SW［・■］の場合、
	//	・CT-ARKメニューモードで動作する（"ct-ark.sys"を読込んで実行する）
	//	・"ct-ark.sys"が読み込めなかった場合は、MAPPERE RAMモードで動作する
	else {
		// microSDからメニュープログラムを読み込んで実行する
		if( loadSysProgram("ct-ark2.sys") ) {
			g_RomType = SETROMTYPE::CTARKMENU;
		}
		else {
			// "ct-ark.sys"がなかった場合は、0x8010 番地以降にその旨を書き込んでおく
			auto *p = g_WorkRam.GetPtrPage(0, 1);
			memcpy((void*)&p[0x10], "not found ct-ark2.sys", 20);
		}
	}

#ifdef FOR_DEBUG_PRINT
	printf("RomType %c\n", g_RomType);
#endif

 	// Blue LED(D1)
	gpio_put(
		GP_PICO_LED,
		(g_RomType==SETROMTYPE::MAPPERERAM)? LED_OFF: LED_ON);

	// 
	multicore_launch_core1(Core1Task);
	Core0Task();

	for(;;){
		// do nothing
	}

	return 0;
}
