#pragma once
#include <stdint.h>

#pragma pack(push,1)

enum class IFCMD : uint8_t
{
	NONE			= 0x00,	// none
	UPDATE_INFO		= 0x01,	// Request infomation
	UPDATE_FILELIST	= 0x02,	// Request update file-list
	REQ_FILELIST	= 0x03,	// Request file-list
	SET_ROM			= 0x04,	// Set number of boot rom file, 0:No Select, 1-54:Selected No
	DEL_ROMFILE		= 0x05,	// Delete file, 1-54:Selected No
	WR_FILE			= 0x06,	// ROM->FILE, blockno, name strings, block data
	BAS2ROM			= 0x07,	// BASIC Program file -> ROM File, Selected No
	STORE_ROM		= 0x08,	// FILE->64K Simple ROM Cartridge, Selected No, TopAddr(0000h-), blockno
	SET_FREQ		= 0x09,	// Set system clock frequency, 125-300MHz
	CLEAR_E_PAPER	= 0x0A, // Clear E-Paper
	PRINT_E_PAPER	= 0x0B, // Set number of BMP file for E-Paper.
};

//enum class SETROMTYPE : uint8_t
// global.h で定義されている

struct MSXPICOIF_FILERECORD	// 18 bytes
{
	uint16_t	no;						// ROMファイル管理番号 1～
	uint8_t		padding;				// (不使用)
	uint8_t		name[LEN_FILE_NAME+1];	// name	  ex) "GAME.ROM" 終端は\0
	uint16_t	fsize;					// ファイルサイズ(KB単位、端数切り上げ）
};

static const int PAGE_FILERECORDS = 3*18;

// msxからpicoにsdcard内のファイルのリストを作成するよう要求する
struct STR_UPDATE_FILELIST
{
	IFCMD					code;					// IFCMD::REQ_FILELIST
	uint8_t					action;					// 不使用
	// 以下はpico->msx
	uint8_t					status;					// 0:no sd card, !0:files
	uint16_t				total_num_files;		// 総ファイル数 0～999
};

struct STR_REQ_FILELIST
{
	IFCMD					code;					// IFCMD::REQ_FILELIST
	uint8_t					action;					// 不使用
	uint16_t				reqTopNo;				// 要求先頭ファイル番号 1～
	// 以下はpico->msx
	uint8_t					status;					// 0:no sd card, !0:files
	uint8_t					padding1;				// 不使用 0固定
	uint8_t					padding2;				// 不使用 0固定
	MSXPICOIF_FILERECORD	files[PAGE_FILERECORDS];// ファイル一覧
};

struct STR_SET_ROM
{
	IFCMD	code;			// IFCMD::SET_ROM
	uint8_t	file_no;		// 選択ファイル番号 0:選択無し 1～:ROMファイル番号
	uint8_t boot_rom_type;	// SETROMTYPE
};

struct STR_DEL_ROMFILE
{
	IFCMD	code;			// IFCMD::DEL_ROMFILE
	uint8_t	file_no;		// 削除したいROMファイル番号 1～
};

struct STR_SET_FREQ
{
	IFCMD	code;			// IFCMD::SET_FREQ
	uint8_t	freq_mhz;		// 0～255 = 周波数MHz 125～380MHz
};

static const int WR_FILE_BLOSK_SIZE = 0x2000;
struct STR_WR_FILE
{
	IFCMD	code;							// IFCMD::WR_FILE
	uint8_t	block_no;						// ブロック単位の番号 0～
	uint8_t	cmd_fwr_fspec[LEN_FILE_SPEC+1];	// 保存ファイル名(拡張子無し）
	uint8_t	block_data[WR_FILE_BLOSK_SIZE];	// ブロックデータ
};

struct STR_BAS2ROM
{
	IFCMD	code;							// IFCMD::BAS2ROM
	uint8_t	file_no;						// ROMファイルに変換したいBASファイルの番号 1～
	uint8_t	cmd_fwr_fspec[LEN_FILE_SPEC+1];	// 保存ファイル名(拡張子無し）
};

static const int STORE_ROM_BLOSK_SIZE = 0x2000;
struct STR_STORE_ROM
{
	IFCMD	code;					// IFCMD::STORE_ROM
	uint8_t	file_no;				// ROMへ書き込みたいROMファイルの番号 1～
	uint8_t	block_no;						// ブロック単位の番号 0～
	// (Pico->MSX)Picoは要求されたブロック番号を下記に格納する(16KB単位)
	uint8_t	err_no;					// 0:ファイル読み込みOK、1:何らかの理由で読み込みできず
	uint8_t	str_num_total_block;	// ファイル全体のブロック数
	uint8_t	str_block;				// 0:要求されたブロックは無し、1:有り
	uint8_t	str_block_data[STORE_ROM_BLOSK_SIZE];	// ブロックデータ
};

struct STR_PRINT_EPAPER
{
	IFCMD	code;			// IFCMD::SET_ROM
	uint8_t	file_no;		// 選択ファイル番号 0:選択無し 1～:ROMファイル番号
};

union CMDS
{
	IFCMD				code;				// （コマンドコードの参照）
	STR_UPDATE_FILELIST	update_filelist;	// SDカード内のROM、BASファイルのリストを作成しなおす
	STR_REQ_FILELIST	req_filelist;		// SDカード内のROM、BASファイルのリストをPicoに要求する
	STR_SET_ROM			set_rom;			// Lモードで起動するROMイメージのファイルをPicoに伝える
	STR_DEL_ROMFILE		del_romfile;		// 指定のROMファイルをSDカードから削除するようPicoに要求する
	STR_WR_FILE			wr_file;			// ROMから読み出したイメージデータをファイル保存ようPicoに伝える
	STR_BAS2ROM			bas2rom;			// BASファイル→ROMファイル変換(8000hスタート、最大16KB)をPicoに伝える
	STR_STORE_ROM		store_rom;			// ROMへ書き込むデータをPicoに要求する
	STR_SET_FREQ		set_freq;			// Picoのシステムクロック周波数を設定する
	STR_PRINT_EPAPER	print_epaper;		// E-Paperの印刷
};

struct MSXPICO_IF
{
	uint8_t	magic_word[12];			// msx -> pico	"OPEN THE ARK"
	// infomation
	uint8_t	pico_side_version[5+1];	// msx <- pico	"xx.xx" + \0
	uint8_t	pico_clock_mhz[5+1];	// msx <- pico	"(xxx)" Mhz
	uint8_t	boot_rom_name[LEN_FILE_NAME+1];	// msx <- pico	"xxxxxxxx.xxx" + \0
	// command
	IFCMD	cmd_res;				// msx <- pico
	CMDS	cmds;					// msx -> pico
};

#pragma pack(pop)


