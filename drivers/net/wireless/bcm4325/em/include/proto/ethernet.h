/*
 * From FreeBSD 2.2.7: Fundamental constants relating to ethernet.
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: ethernet.h,v 9.45.74.3 2009/11/25 03:14:29 Exp $
 */


#ifndef _NET_ETHERNET_H_	    
#define _NET_ETHERNET_H_

#ifndef _TYPEDEFS_H_
#include "typedefs.h"
#endif


#include <packed_section_start.h>



#define	ETHER_ADDR_LEN		6


#define	ETHER_TYPE_LEN		2


#define	ETHER_CRC_LEN		4


#define	ETHER_HDR_LEN		(ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)


#define	ETHER_MIN_LEN		64


#define	ETHER_MIN_DATA		46


#define	ETHER_MAX_LEN		1518


#define	ETHER_MAX_DATA		1500


#define ETHER_TYPE_MIN		0x0600		
#define	ETHER_TYPE_IP		0x0800		
#define ETHER_TYPE_ARP		0x0806		
#define ETHER_TYPE_8021Q	0x8100		
#define	ETHER_TYPE_BRCM		0x886c		
#define	ETHER_TYPE_802_1X	0x888e		
#ifdef BCMWAPI_WPI
#define ETHER_TYPE_WAI		0x88b4		
#endif 
#ifdef BCMWPA2
#define	ETHER_TYPE_802_1X_PREAUTH 0x88c7	
#endif


#define	ETHER_BRCM_SUBTYPE_LEN	4	
#define	ETHER_BRCM_CRAM		1	


#define ETHER_DEST_OFFSET	(0 * ETHER_ADDR_LEN)	
#define ETHER_SRC_OFFSET	(1 * ETHER_ADDR_LEN)	
#define ETHER_TYPE_OFFSET	(2 * ETHER_ADDR_LEN)	


#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)


#ifndef __INCif_etherh     

BWL_PRE_PACKED_STRUCT struct ether_header {
	uint8	ether_dhost[ETHER_ADDR_LEN];
	uint8	ether_shost[ETHER_ADDR_LEN];
	uint16	ether_type;
} BWL_POST_PACKED_STRUCT;


BWL_PRE_PACKED_STRUCT struct	ether_addr {
	uint8 octet[ETHER_ADDR_LEN];
} BWL_POST_PACKED_STRUCT;
#endif	


#define ETHER_SET_LOCALADDR(ea)	(((uint8 *)(ea))[0] = (((uint8 *)(ea))[0] | 2))
#define ETHER_IS_LOCALADDR(ea) 	(((uint8 *)(ea))[0] & 2)
#define ETHER_CLR_LOCALADDR(ea)	(((uint8 *)(ea))[0] = (((uint8 *)(ea))[0] & 0xd))
#define ETHER_TOGGLE_LOCALADDR(ea)	(((uint8 *)(ea))[0] = (((uint8 *)(ea))[0] ^ 2))


#define ETHER_SET_UNICAST(ea)	(((uint8 *)(ea))[0] = (((uint8 *)(ea))[0] & ~1))


#define ETHER_ISMULTI(ea) (((const uint8 *)(ea))[0] & 1)



#define	ether_cmp(a, b)	(!(((short*)a)[0] == ((short*)b)[0]) | \
			 !(((short*)a)[1] == ((short*)b)[1]) | \
			 !(((short*)a)[2] == ((short*)b)[2]))


#define	ether_copy(s, d) { \
		((short*)d)[0] = ((short*)s)[0]; \
		((short*)d)[1] = ((short*)s)[1]; \
		((short*)d)[2] = ((short*)s)[2]; }


#define ETHER_ISBCAST(ea) ((((uint8 *)(ea))[0] &		\
			    ((uint8 *)(ea))[1] &		\
			    ((uint8 *)(ea))[2] &		\
			    ((uint8 *)(ea))[3] &		\
			    ((uint8 *)(ea))[4] &		\
			    ((uint8 *)(ea))[5]) == 0xff)

static const struct ether_addr ether_bcast = {{255, 255, 255, 255, 255, 255}};
static const struct ether_addr ether_null = {{0, 0, 0, 0, 0, 0}};


#define ETHER_ISNULLADDR(ea) ((((uint8 *)(ea))[0] |		\
			    ((uint8 *)(ea))[1] |		\
			    ((uint8 *)(ea))[2] |		\
			    ((uint8 *)(ea))[3] |		\
			    ((uint8 *)(ea))[4] |		\
			    ((uint8 *)(ea))[5]) == 0)



#include <packed_section_end.h>

#endif 
