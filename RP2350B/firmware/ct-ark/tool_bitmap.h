#pragma once

/**
 * BITMAP tools
 * Copyright (c) 2026 Harumakkin.
 * SPDX-License-Identifier: MIT
 */
// https://spdx.org/licenses/


#include "bitmap.h"

uint8_t *t_LoadBmp24ToEPDformat128x296(
	const char *pFilePath, uint8_t *pWorkArea, const int workSize);
