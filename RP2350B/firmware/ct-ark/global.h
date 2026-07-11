#pragma once
#include <memory.h>
#include "tools.h"


// メモリ
const int MSX_EXSLOT_NUM = 4;		// 拡張スロット内のスロット数
const int MSX_PAGE_NUM = 4;			// ページ数 / Z80 main memory.
const int MEM_16K_SIZE = 16*1024;
const int MEM_32K_SIZE = 32*1024;
const int MEM_64K_SIZE = 64*1024;
struct MEM_16K
{
	uint8_t mem[MEM_16K_SIZE];
};

struct ARKWORKRAM
{
#if defined(PICO_PLATFORM_RP2350)
	// 全16Kブロックの個数 24*16K = 384KB
	static const int TOTAL_MEM_16K = 24;
#elif defined(PICO_PLATFORM_RP2040)
	// 全16Kブロックの個数 12*16K = 192KB
	static const int TOTAL_MEM_16K = 12;
#endif
	MEM_16K		Blocks[TOTAL_MEM_16K];
	// マッパーセグメントとして使用するブロックの開始番号とブロック数
		int			NumMapperBlocks;
	//  各ページのマッパーセグメントへのポインタ
	uint8_t		*pMemSts[MSX_EXSLOT_NUM][MSX_PAGE_NUM];

	//
	ARKWORKRAM()
	{
		memset(Blocks, 0, sizeof(Blocks));
		NumMapperBlocks = TOTAL_MEM_16K;
	 	for( int t = 0; t < TOTAL_MEM_16K; t++){
			Blocks[t].mem[0] = 0x00;
		}
		SetDefaultMem();
		return;
	}
	virtual ~ARKWORKRAM()
	{
		return;
	}
	int GetSize() const
	{
		return sizeof(MEM_16K)*TOTAL_MEM_16K;
	}
	
	void SetNumMapperBlocks(const int n)
	{
		NumMapperBlocks = n;
		return;
	}

	void SetVoidMem()
	{
		for( int st = 0; st < MSX_EXSLOT_NUM; ++st) {
			for( int pg = 0; pg < MSX_PAGE_NUM; ++pg) {
				pMemSts[st][pg] = nullptr;
			}
		}
		return;
	}

	void SetDefaultMem()
	{
		for( int t = 0; t < MSX_PAGE_NUM; ++t) {
			SetMemMap(0, t, t);
		}
		for( int st = 1; st < MSX_EXSLOT_NUM; ++st) {
			for( int pg = 0; pg < MSX_PAGE_NUM; ++pg) {
				pMemSts[st][pg] = nullptr;
			}
		}
		return;
	}

	inline void SetMemMap(const int slotNo, const int pgNo, const int blockNo)
	{
		pMemSts[slotNo][pgNo] =
			(blockNo < NumMapperBlocks)
			? Blocks[blockNo].mem
			: nullptr;
//		pMemSts[slotNo][pgNo] = Blocks[blockNo % NumMapperBlocks].mem;
		return;
	}

	inline uint8_t *GetPtrPage(const int slotNo, const int pgNo)
	{
		return pMemSts[slotNo][pgNo];
	}

	inline void SetPtrPage(const int slotNo, const int pgNo, uint8_t *p)
	{
		pMemSts[slotNo][pgNo] = p;
		return;
	}
};
extern ARKWORKRAM g_WorkRam;

// CT-ARK特殊動作の状態かどうか
extern volatile bool g_bActiveCtArk;

// ROMの種類
enum class SETROMTYPE : uint8_t
{
	NONE				= '0',
	CTARKMENU			= '1',
	MAPPERERAM			= '2',
	//
	ROM_PAGE_0			= 'a',	// 0000h～
	ROM_PAGE_1			= 'b',	// 4000h～
	ROM_PAGE_2			= 'c',	// 8000h～
	ROM_PAGE_3			= 'd',	// C000h～
	MSXDOS2_320KB		= 'e',	// DOS2カーネル + MapperRAM(320KB)
	ASC8K_RAM			= 'f',	// PICO RAM上での動作
	ASC16K_RAM			= 'g',	// PICO RAM上でASCII16Kの動作
	ASC8K_FLASHROM		= 'h',	// PICO FlushROM上でASCII8Kの動作（384KB以下の場合ははRAM動作になる）
	ASC16K_FLASHROM		= 'i',	// PICO FlushROM上でASCII16Kの動作（384KB以下の場合ははRAM動作になる）
	NEXTOR128KB_256KB	= 'j',	// NEXTORカーネル128KB + MapperRAM(256KB)
	RTYPE				= 'k',	// ASCII16K(R-TYPE)
	// KONAMI8K			= '',
	// KONAMISCC		= '',
	// KONAMISCCI		= '',
	//
};
extern volatile SETROMTYPE g_RomType;
extern volatile uint32_t g_Num8KBlosks;

#if defined(PICO_PLATFORM_RP2350)
#ifdef WAVESHARE_CORE2350B
// WAVESHARE_CORE2350Bは、FlashROMは16MBある。
// プログラムコードが最大でも、訳512KBと仮定し、残り、訳15.5MBをROMイメージの格納領域にする。
const uint32_t RP2350_FLASHROM_SIZE = 16*1024*1024;
#else
// RaspiPico2は、FlashROMは4MBある。
// プログラムコードが最大でも、訳512KBと仮定し、残り、訳3.5MBをROMイメージの格納領域にする。
const uint32_t RP2350_FLASHROM_SIZE = 4*1024*1024;
#endif
const uint32_t TOPADDR_FLASHROM_IMAGE = 0x080000;
const uint32_t SIZE_FLASHROM_IMAGE = RP2350_FLASHROM_SIZE - TOPADDR_FLASHROM_IMAGE;
const uint32_t TOPADDR_CTARK_INFO = TOPADDR_FLASHROM_IMAGE-4096;
#elif defined(PICO_PLATFORM_RP2040)
const uint32_t RP2040_FLASHROM_SIZE = 2*1024*1024;
const uint32_t TOPADDR_FLASHROM_IMAGE = 0x080000;
const uint32_t SIZE_FLASHROM_IMAGE = RP2040_FLASHROM_SIZE - TOPADDR_FLASHROM_IMAGE;
const uint32_t TOPADDR_CTARK_INFO = TOPADDR_FLASHROM_IMAGE-4096;
#endif

struct FR_BOOTROM_INFO final
{
public:
	uint8_t		FileName[LEN_FILE_NAME+1];	// ファイル名情報(8.3形式)
	SETROMTYPE	RomType;
	uint32_t	Num8KBlosks;				// 8KB単位の占有ブロック数
	uint32_t	FileSize;					// ファイルサイズ[Bytes]
	//
	FR_BOOTROM_INFO()
	{
		Clear();
		return;
	}

	void Clear()
	{
		FileName[0] = '\0';
		RomType = SETROMTYPE::NONE;
		Num8KBlosks = 0;
		FileSize = 0;
		return;
	}
};


const static uint8_t g_INFOSIGN[6] = {'C', 'T', 'A', 'R', 'K', '\0'};

struct FR_CTARK_INFO final
{
public:
	// -------------------------------
#pragma pack(push, 1)
	// シグネイチャ領域
	uint8_t		Signature[6];			// "CTARK\0"
	// システムクロック周波数(MHz)
	uint16_t	SysClk;
	// パディング
	uint8_t		Padding[24];
	// 起動ROM情報
	FR_BOOTROM_INFO BootRomInfo;
#pragma pack(pop)
	// -------------------------------

	//
	FR_CTARK_INFO()
	{
		ResetInfo();
		return;
	}

	void ResetInfo()
	{
		memcpy(Signature, g_INFOSIGN, sizeof(g_INFOSIGN));
		// デフォルトの速度
		SysClk = GetDefaultSysClk();
		//
		BootRomInfo.Clear();
		return;
	}

	bool CheckInfo() const
	{
		if( memcmp(Signature, g_INFOSIGN, sizeof(g_INFOSIGN)) != 0)
			return false;
		if( SysClk < 125 || 300 < SysClk)
			return false;
		return true;
	}
	uint16_t GetDefaultSysClk() const
	{
		uint16_t clk = 125;
#if defined(PICO_PLATFORM_RP2350)
		clk = 150;
#elif defined(PICO_PLATFORM_RP2040)
		clk = 125;
#endif
		return clk;
	}
};

extern uint8_t g_CtArkInfoSector[256];
extern FR_CTARK_INFO *g_pCAI;
//extern FR_CTARK_TEMP_MODE g_CM;