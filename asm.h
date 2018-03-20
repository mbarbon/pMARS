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
 * asm.h: header for asm.c
 * $Id: asm.h,v 2.13 1996/02/20 19:15:26 stst Exp stst $
 */

#define NONE      0
#define ADDRTOKEN 1
#define FSEPTOKEN 2
#define COMMTOKEN 3
#define MODFTOKEN 4
#define EXPRTOKEN 5
#define WSPCTOKEN 6
#define CHARTOKEN 7
#define NUMBTOKEN 8
#define APNDTOKEN 9
#define MISCTOKEN 10                /* unrecognized token */

#define sep_sym ','
#define com_sym ';'
#define mod_sym '.'
#define cat_sym '&'
#define cln_sym ':'

extern char addr_sym[], expr_sym[], spc_sym[];
extern char *opname[], *modname[];

#if defined(NEW_STYLE)
extern char *pstrdup(char *), *pstrcat(char *, char *), *pstrchr(char *, int);
extern uChar ch_in_set(uShrt, char *), skip_space(char *, uShrt);
extern uChar str_in_set(char *, char *s[]);
extern int get_token(char *, uChar *, char *);
extern void to_upper(char *);
#else
extern char *pstrdup(), *pstrcat(), *pstrchr();;
extern uChar ch_in_set(), skip_space();
extern uChar str_in_set();
extern int get_token();
extern void to_upper();
#endif
