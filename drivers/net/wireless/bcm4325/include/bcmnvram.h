/*
 * NVRAM variable manipulation
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: bcmnvram.h,v 13.58.114.1 2008/05/07 19:52:33 Exp $
 */


#ifndef _bcmnvram_h_
#define _bcmnvram_h_

#ifndef _LANGUAGE_ASSEMBLY

#include <typedefs.h>
#include <bcmdefs.h>

struct nvram_header {
	uint32 magic;
	uint32 len;
	uint32 crc_ver_init;	
	uint32 config_refresh;	
	uint32 config_ncdl;	
};

struct nvram_tuple {
	char *name;
	char *value;
	struct nvram_tuple *next;
};


extern char *nvram_default_get(const char *name);


extern int nvram_init(void *sih);


extern int nvram_append(void *si, char *vars, uint varsz);


extern bool nvram_reset(void *sih);


extern void nvram_exit(void *sih);


extern char * nvram_get(const char *name);


extern int BCMINITFN(nvram_resetgpio_init)(void *sih);


#define nvram_safe_get(name) (nvram_get(name) ? : "")


static INLINE int
nvram_match(char *name, char *match) {
	const char *value = nvram_get(name);
	return (value && !strcmp(value, match));
}


static INLINE int
nvram_invmatch(char *name, char *invmatch) {
	const char *value = nvram_get(name);
	return (value && strcmp(value, invmatch));
}


extern int nvram_set(const char *name, const char *value);


extern int nvram_unset(const char *name);


extern int nvram_commit(void);


extern int nvram_getall(char *nvram_buf, int count);


uint8 nvram_calc_crc(struct nvram_header * nvh);

#endif 


#define NVRAM_SOFTWARE_VERSION	"1"

#define NVRAM_MAGIC		0x48534C46	
#define NVRAM_CLEAR_MAGIC	0x0
#define NVRAM_INVALID_MAGIC	0xFFFFFFFF
#define NVRAM_VERSION		1
#define NVRAM_HEADER_SIZE	20
#define NVRAM_SPACE		0x8000

#define NVRAM_MAX_VALUE_LEN 255
#define NVRAM_MAX_PARAM_LEN 64

#define NVRAM_CRC_START_POSITION	9 
#define NVRAM_CRC_VER_MASK	0xffffff00 

#endif 
