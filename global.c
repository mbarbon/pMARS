/* portable MARS, Copyright (c) 1993-1996
 *
 * pMARS, a Memory Array Redcode Simulator which supports:
 * - ICWS'88 and ICWS'94 compatibility.
 * - Programmable debugger.
 * - Turbo simulator.
 * - Graphics for many platforms.
 *
 * Contributors:
 * Albert Ma               (ama@mit.edu)
 * Na'ndor Sieben          (sieben@imap1.asu.edu)
 * Stefan Strack           (stst@vuse.vanderbilt.edu)
 * Mintardjo Wangsawidjaja (wangsawm@kira.csos.orst.edu)
 *
 * global.c: global data declarations
 * $Id: global.c,v 1.18 1996/02/20 19:15:36 stst Exp stst $
 */

#ifndef _GLOBAL_INCLUDED
#include "global.h"
#endif

/* We might need this var */
int     errorcode = SUCCESS;
int     errorlevel = WARNING;
char    errmsg[MAXALLCHAR];

/* Some parameters */
int     warriors;
ADDR_T  coreSize;
int     taskNum;
ADDR_T  instrLim;
ADDR_T  separation;
int     rounds;
long    cycles;

int     cmdMod = 0;                /* cdb command flag: 0, RESET, SKIP */
S32_T   seed;

int     SWITCH_e;
int     SWITCH_b;
int     SWITCH_k;
int     SWITCH_8;
int     SWITCH_f;
ADDR_T  SWITCH_F;
int     SWITCH_V;
int     SWITCH_o;
int     SWITCH_Q = -1;                /* not set */
char   *SWITCH_eq = DEFAULTSCORE;
#ifdef VMS
int     SWITCH_D;
#endif

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
int     SWITCH_v;
int     displayLevel;
int     displayMode;
int     displaySpeed;
#if defined(CURSESGRAPHX)
/* This variable determines how often the screen is refreshed (wrefresh()).
   10 means the screen is refreshed every 20th cycle. Higher numbers mean
   faster but jerkier display */
int     refreshInterval;
int     refIvalAr[SPEEDLEVELS] = {50, 20, 10, 3, 2, 1, 1, 1, 1};
#else
int     keyDelay;
#if defined(XWINGRAPHX)
int     keyDelayAr[SPEEDLEVELS] = {255, 20, 0, 0, 0, 0, 0, 0, 0};
#else
int     keyDelayAr[SPEEDLEVELS] = {25, 20, 0, 0, 0, 0, 0, 0, 0};
#endif
unsigned long loopDelay;
unsigned long loopDelayAr[SPEEDLEVELS] = {1, 1, 1, 100, 500, 2500, 10000, 40000, 100000};
#endif
#endif

int     inCdb = FALSE;
int     debugState = NOBREAK;
int     copyDebugInfo = TRUE;
#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
int     inputRedirection = FALSE;
#endif
mem_struct INITIALINST;                /* initialize to DAT.F $0,$0 */

warrior_struct warrior[MAXWARRIOR];
#ifdef DOS16
ADDR_T far *pSpace[MAXWARRIOR];
#else
ADDR_T *pSpace[MAXWARRIOR];
#endif
ADDR_T  pSpaceSize;
