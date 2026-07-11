#include "pico/stdlib.h"	// for uint8_t
#include "global.h"


// メモリ管理
ARKWORKRAM g_WorkRam;
volatile bool g_bActiveCtArk;

// FlashROMに関する
uint8_t g_CtArkInfoSector[256];
// g_pCAIは、FR_CTARK_INFOで直接定義せずに、uint8_t[256]の領域を必ず持つように
// するためこの定義方法を採用している
FR_CTARK_INFO *g_pCAI = reinterpret_cast<FR_CTARK_INFO*>(g_CtArkInfoSector);
volatile SETROMTYPE g_RomType;
volatile uint32_t g_Num8KBlosks;

