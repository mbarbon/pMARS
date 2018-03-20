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
 * sim.h: header for sim.c, cdb.c and display files
 * $Id: sim.h,v 2.8 1996/02/20 19:15:49 stst Exp stst $
 */

/*
 * IR=memory[progCnt]
 * for the currently executing warrior
 * progCnt is the current cell being executed (operand decoding has not
 * occured yet)
 * W->taskHead is the next instruction in the queue (after the current inst)
 * W->tasks is the number of processes running
 * for the other warrior:  W->nextWarrior
 * W->nextWarrior->taskHead is the next instruction in the queue
 * W->nextWarrior->tasks is the number of processes running
 */

#ifndef SIM_INCLUDED
#define SIM_INCLUDED
#endif

#ifdef DOSTXTGRAPHX
#define         HELP_PAGE 3
#define         CDB_PAGE 5
#define         CORE_PAGE 1
#define         DEF_PAGE 0
#define         NORMAL_ATTR 0x0700
#ifdef CURSESGRAPHX
#define         show_cursor()
#define         hide_cursor()
#else
#define         show_cursor() SetCursor(7,8)
#define         hide_cursor() SetCursor(-1,-1)
#endif
#endif
#ifdef DOSGRXGRAPHX
extern int bgiTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#ifdef LINUXGRAPHX
extern int svgaTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#ifdef XWINGRAPHX
extern int xWinTextLines;
extern int printAttr;
#define         WAIT 1
#define         NOWAIT 0
#endif

#if defined(DOSTXTGRAPHX) || defined(DOSGRXGRAPHX) || defined(LINUXGRAPHX) \
    || defined(XWINGRAPHX)
#define         PC1_ATTR 1
#define         PC2_ATTR 2
#endif                                /* tells grputs that print_core() is printing
                                 * PC1 or 2 */

#if defined(DOSALLGRAPHX)
extern int displayMode;
#endif

#ifdef DOS16
#define FAR far
#else
#define FAR
#endif

extern int round;
extern long cycle;
extern ADDR_T progCnt;                /* program counter */
extern warrior_struct *W;        /* indicate which warrior is running */
extern char alloc_p;
extern int warriorsLeft;

extern ADDR_T FAR *endQueue;
extern mem_struct FAR *memory;
extern ADDR_T FAR *taskQueue;
extern warrior_struct *endWar;
extern U32_T totaltask;
