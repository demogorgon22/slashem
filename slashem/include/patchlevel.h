/*	SCCS Id: @(#)patchlevel.h	3.4	2003/08/21	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*The name of the compiled game- should be same as stuff in makefile*/
/*for makedefs*/
/* KMH -- Made it mixed case, from which upper & lower case versions are made */
#define DEF_GAME_NAME   "SlashEM"
/*#define DEF_GAME_NAME   "NetHack"*/

/* Version */
#define VERSION_MAJOR   0
#define VERSION_MINOR   0
/*
 * PATCHLEVEL is updated for each release.
 */
#define PATCHLEVEL      7
#define EDITLEVEL	5
/* #define FIXLEVEL        0 */

#define COPYRIGHT_BANNER_A \
"This is SuperLotsoAddedStuffHack-Extended Magic 1997-2003"

#define COPYRIGHT_BANNER_B \
"NetHack, Copyright 1985-2003 Stichting Mathematisch Centrum, M. Stephenson."

#define COPYRIGHT_BANNER_C \
"See license for details. Bug reports to slashem-discuss@lists.sourceforge.net"

#if 1
/*
 * If two successive patchlevels have compatible data files (fat chance),
 * defining this with the value of the older one will allow its bones and
 * save files to work with the newer one.  The format is
 *	0xMMmmPPeeL
 * 0x = literal prefix "0x", MM = major version, mm = minor version,
 * PP = patch level, ee = edit level, L = literal suffix "L",
 * with all four numbers specified as two hexadecimal digits.
 */
#define VERSION_COMPATIBILITY 0x00000702L
#endif

/*patchlevel.h*/
