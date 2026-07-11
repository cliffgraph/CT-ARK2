/**
 * CT-ARK (RaspberryPiPico firmware)
 * Copyright (c) 2023 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/

//#include <stdio.h>
#include <memory.h>
#include <stdint.h>
#include "tools.h"

// ファイル名に使用できる文字かどうかチェックする
bool t_CheckFilespecChar(const uint8_t ch)
{
	if(('0' <= ch && ch <= '9') )
		return true;
	if(('a' <= ch && ch <= 'z') )
		return true;
	if(('A' <= ch && ch <= 'Z') )
		return true;
	const char *pChars = "$&#@!%'()-{}~_";
	for( int t = 0; pChars[t] != '\0'; ++t ) {
		if( pChars[t] == (char)ch )
			return true;
	}
	return false;
}

void t_SplitPath(const char *pPath, STRFILENAME *pStr)
{
	if( pStr == nullptr )
		return;
	memset(pStr->spec, '\0', sizeof(pStr->spec));
	memset(pStr->ext, '\0', sizeof(pStr->ext));
	int src = 0;
	for(int dest = 0; pPath[src] != '.'; ++src, ++dest) {
		if( pPath[src] == '\0' ) 
			return;
		pStr->spec[dest] = pPath[src];
	}
	++src;
	for(int dest = 0; pPath[src]!='\0'; ++src, ++dest) {
		pStr->ext[dest] = pPath[src];
	}
	return;
}

static uint16_t t_GetPtrByLowNo(volatile uint8_t *p, const int tgtNo)
{
	int nextPtr;
	int index = 0;
	index++;
	for(;;) {
		nextPtr = (p[index+1]<<8)|p[index+0];
		if( nextPtr == 0 )
			break;
		const int no = (p[index+3]<<8)|p[index+2];
		if( no == tgtNo ){
			break;
		}
		index = nextPtr - 0x8000;
	}
	return nextPtr;
}

// BASICプログラム(ファイル）をROMファイル化する。
// @note 先頭アドレスは0x8001で作成されたBASICプログラム前提とする
//       BASICプログラム中のアドレス値を、romヘッダの16bytes分後ろへ移動させる(+16する)
bool t_ReassignmentAddress(volatile uint8_t *p)
{
	volatile uint8_t *pSrc = p;
	const uint16_t DIFFADDR = 16;
	uint16_t temp16;
	if( *p != 0xff )
		return false;
	*p = 0x00;
	p++;
	for(;;) {
		// 次行を示すリンクポインタ（0なら修正終了)
		// auto *pTemp16 = reinterpret_cast<volatile uint16_t*>(p);	// ←なぜかハングアップする
		temp16 = (p[1]<<8)|p[0];
		if( temp16 == 0x0000 )
			break;
		temp16 += DIFFADDR;
		p[0] = temp16 & 0xff;
		p[1] = temp16 >> 8;
		p+=2;
		// 行番号
		p+=2;	// 読み捨て
		// (マルチ)ステートメント内の修正
		bool bStr = false;
		bool bRem = false;
		for(;;) {
			//printf( "S %02x\n", *p );
			if( *p == 0x00 ){	// 終端なら修正終了
				p++;
				break;
			}
			if( bRem ) {		// 終端まで読み捨て
				p++;
			}
			else if( bStr ){
				if( *p == '\"' )
					bStr = false;
				p++;
			}
			else {
				switch(*p++)
				{
					case '\"':	bStr = true; break;
					case 0x01:	p+=1;	break;	// グラフィック文字
					case 0x0b:	p+=2;	break;	// 8進数
					case 0x0c:	p+=2;	break;	// 16進数
					case 0x0d:	// 行番号(絶対アドレス)
						// pTemp16 = reinterpret_cast<volatile uint16_t*>(p);
						temp16 = (p[1]<<8)|p[0];
						temp16 += DIFFADDR;
						p[0] = temp16 & 0xff;
						p[1] = temp16 >> 8;
						p+=2;
						break;
					case 0x0e:	// 行番号(行番号)は、
					{
						uint16_t tgtNo = (p[1]<<8)|p[0];
						temp16 = t_GetPtrByLowNo(pSrc, tgtNo);
						if( temp16 != 0x0000 ){
							temp16 += DIFFADDR;
							p[-1] = 0x0d;
							p[0] = temp16 >> 8;
							p[1] = temp16 & 0xff;
						}
						p+=2;
						break;
					}
					case 0x0f:	p+=1;	break;	// 整数 10～255
					case 0x1c:	p+=2;	break;	// 整数 256～32767
					case 0x1d:	p+=4;	break;	// 単精度実数
					case 0x1f:	p+=8;	break;	// 倍精度実数
					case 0x26:					// &B二進数
						if( *p == 'B' ){
							p++;
							while(*p=='0'||*p=='1') p++;
						}
						break;
					case 0x3a:
						if( *p == 0xa1 ){		// ELSE
							p++;
							break;
						}
						if( *p == 0x8f ){
							p++;
							if( *p == 0xe6 ) {	// '
								bRem = true;
								p++;
							}
						}
						break;
					case 0x8f:	// REM
						bRem = true;
						break;
					case 0xff:
						if( (*p == 0x3e) ||
							(0x81 <= *p && *p <= 0x8d) || 
							(0x8f <= *p && *p <= 0xAf) || 
							(*p == 0xb0) ) {
								p++;
						}
						break;
					default:
						break;
				}
			}
		}
	}
	return true;
}