#pragma once
/**
 * BITMAP 
 * Copyright (c) 2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/


#include <stdio.h>
#include <stdint.h>		// for uint8_t

#pragma pack(push, 1)

struct _BITMAPFILEHEADER
{
    uint16_t bfType;        // "BM" = 0x4D42
    uint32_t bfSize;        // ファイルサイズ
    uint16_t bfReserved1;   // 0
    uint16_t bfReserved2;   // 0
    uint32_t bfOffBits;     // 画像データまでのオフセット
};

struct _BITMAPINFOHEADER
{
    uint32_t biSize;          // 構造体サイズ（40）
    int32_t  biWidth;         // 幅
    int32_t  biHeight;        // 高さ
    uint16_t biPlanes;        // 常に1
    uint16_t biBitCount;      // 色深度
    uint32_t biCompression;   // 圧縮方式
    uint32_t biSizeImage;     // 画像データサイズ
    int32_t  biXPelsPerMeter; // 水平解像度
    int32_t  biYPelsPerMeter; // 垂直解像度
    uint32_t biClrUsed;       // パレット色数
    uint32_t biClrImportant;  // 重要色数
};

struct _RGBQUAD
{
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
    uint8_t rgbReserved;
};
#pragma pack(pop)

typedef struct _BITMAPFILEHEADER BITMAPFILEHEADER;
typedef struct _BITMAPINFOHEADER BITMAPINFOHEADER;
typedef struct _RGBQUAD RGBQUAD;


