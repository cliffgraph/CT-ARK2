#pragma once

bool t_CheckFilespecChar(const uint8_t ch);

// ファイル名の文字長
static const int LEN_FILE_SPEC = 8;
static const int LEN_FILE_EXT = 3;
static const int LEN_FILE_NAME = LEN_FILE_SPEC+1+LEN_FILE_EXT;

struct STRFILENAME
{
	char spec[LEN_FILE_SPEC+1];
	char ext[LEN_FILE_EXT+1];
};
void t_SplitPath(const char *pPath, STRFILENAME *pStr);

//
bool t_ReassignmentAddress(volatile uint8_t *p);

//
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
