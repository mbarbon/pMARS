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
 * alldisp.c: functions for combined DOS graphics/text mode display
 * $Id: alldisp.c,v 1.4 1996/02/20 19:15:24 stst Exp stst $
 */

void    display_init(void);
void    display_read(int addr);
void    display_close(void);

void
display_init()
{
  if (displayMode == TEXT)
    text_display_init();
  else
    bgi_display_init();
}

void
display_read(addr)
{
  if (displayLevel > 3)
    if (displayMode == TEXT)
      text_display_read(addr);
    else
      bgi_display_read(addr);
}


#define display_dec(addr)\
do {\
if (displayLevel > 2)\
if (displayMode==TEXT) text_display_dec(addr);\
  else bgi_display_dec(addr);\
} while (0)

#define display_inc(addr)\
do {\
if (displayLevel > 2)\
if (displayMode==TEXT) text_display_inc(addr);\
  else bgi_display_inc(addr);\
} while (0)

#define display_write(addr)\
do {\
if (displayLevel > 1)\
if (displayMode==TEXT) text_display_write(addr);\
  else bgi_display_write(addr);\
} while (0)

#define display_exec(addr)\
do {\
if (displayLevel > 0)\
if (displayMode==TEXT) text_display_exec(addr);\
  else bgi_display_exec(addr);\
} while (0)

#define display_spl(warrior,tasks) \
do {\
  if (displayMode==TEXT) {\
    if (displayLevel > 0)\
        text_display_spl(warrior,tasks);\
  } else bgi_display_spl(warrior,tasks);\
} while (0)

#define display_dat(addr,warNum,tasks) \
do {\
  if (displayMode==TEXT) {\
    if (displayLevel > 0)\
        text_display_dat(addr);\
  } else bgi_display_dat(addr,warNum,tasks);\
} while (0)

#define display_clear() \
do {\
if (displayMode==TEXT) text_display_clear();\
  else bgi_display_clear();\
} while (0)

void
display_close()
{
  if (displayMode == TEXT)
    text_display_close();
  else
    bgi_display_close(WAIT);
}
