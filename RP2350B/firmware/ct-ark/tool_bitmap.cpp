/**
 * BITMAP tools
 * Copyright (c) 2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/


#include "tool_bitmap.h"
#include "sdfat.h"
#include <utility> 

struct STR_BITMAPFILE
{
    BITMAPFILEHEADER file;
    BITMAPINFOHEADER info;
};

/**
 * 24bit BMPファイルを読み込んで、電子ペーパー用のbitmapに変換する
 */
uint8_t *t_LoadBmp24ToEPDformat128x296(
	const char *pFilePath, uint8_t *pWorkArea, const int workSize)
{
	UINT readSize;
	if( !sd_fatReadFileFrom(pFilePath, sizeof(STR_BITMAPFILE), pWorkArea, &readSize) ) {
		return nullptr;
	}
	if( readSize != sizeof(STR_BITMAPFILE) ) {
		return nullptr;
	}
	auto *pBmp = reinterpret_cast<const STR_BITMAPFILE *>(pWorkArea);

    // "BM" チェック
    if (pBmp->file.bfType != 0x4D42 || pBmp->info.biBitCount != 24 || pBmp->info.biCompression != 0) {
        return nullptr;
	}

    const int width  = pBmp->info.biWidth;
    const int height = pBmp->info.biHeight;
    const int padding = (4 - ((width*3) % 4)) % 4;
    const int row_bytes = width*3 + padding;
	const uint32_t offsetOfPix = pBmp->file.bfOffBits;
	const uint32_t sizeImage = pBmp->info.biSizeImage;

	// uint32_t sz = pBmp->file.bfSize - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
	// printf("height=%d, row_bytes=%d, bfSize=%d, biSizeImage=%d, %d\n", height, row_bytes, pBmp->file.bfSize, pBmp->info.biSizeImage, sz);
	// printf("bfOffBits=%d\n", pBmp->file.bfOffBits);
	//return nullptr;

	// ピクセルデータの読み込み
	if( !sd_fatReadFileFromOffset(pFilePath, offsetOfPix, sizeImage, pWorkArea, &readSize) ) {
		return nullptr;
	}

    // BMP は下から上へ格納されているのでまず、上から下への順に並び替える
	// heightは偶数であることが前提です。
    for (int y = 0; y < height/2; y++) {
		const int y1 = y;
		const int y2 = height - 1 - y;
		for( int wx = 0; wx < width*3; ++wx) {
			uint8_t *pY1 = &pWorkArea[row_bytes*y1 + wx];
			uint8_t *pY2 = &pWorkArea[row_bytes*y2 + wx];
			std::swap(*pY1, *pY2);
		}
	}

	// 電子ペーパーで使用できる書式に変換して上書きしていく
	int destByte = 0, destBit = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
			const int src = row_bytes*y + 3*x;
			const uint8_t B = pWorkArea[src+0];
			const uint8_t G = pWorkArea[src+1];
			const uint8_t R = pWorkArea[src+2];
			uint8_t c;
				 if( 127<R && 127<G  && 127<B  ) c = 1;		// White
			else if( 127<R && 127<G  && B<=127 ) c = 2;		// Yellow
			else if( 127<R && G<=127 && B<=127 ) c = 3;		// Red
			else 								 c = 0;		// Black
			// 
			uint8_t *pD = &pWorkArea[destByte];
			if(destBit == 0)
				*pD = 0;
			*pD |= c << ((3-destBit)*2);		// 最上位bitがxの若番であることに注意
			// 
			destBit = (destBit+1) % 4;
			if(destBit == 0)
				destByte++;
        }
    }
    return pWorkArea;
}

