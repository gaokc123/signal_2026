#ifndef __print_H__

#define __print_H__

#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <string.h>

int fputc(int c, FILE* stream);
int fputs(const char* restrict s, FILE* restrict stream);
int puts(const char *ptr);

#endif