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
 * $Id: token.c,v 2.15 1996/02/20 19:15:51 stst Exp stst $
 */

#include <ctype.h>
#include <string.h>

#include "global.h"
#include "asm.h"

/* **************************** Prototype ******************************** */

#ifdef NEW_STYLE
#define toupper_(x) (toupper(x))
#else
#define toupper_(x) (isalpha(x) && islower(x) ? toupper(x) : (x))
#endif

/* *************************** definitions ******************************* */

#define is_addr(ch)  pstrchr(addr_sym, (int) ch)
#define is_expr(ch)  pstrchr(expr_sym, (int) ch)
#define is_alpha(ch) (isalpha(ch) || (ch) == '_')

/* ********************************************************************** */

char   *
pstrchr(s, c)
  char   *s;
  int     c;
{
  do {
    if ((int) *s == c)
      return s;
  } while (*s++);

  return NULL;
}

/* ********************************************************************** */

char   *
pstrdup(s)
  char   *s;
{
  char   *p, *q;
  register int i;

  for (q = s, i = 0; *q; q++)
    i++;

  if ((p = (char *) MALLOC(sizeof(char) * (i + 1))) != NULL) {
    q = p;
    while (*s)
      *q++ = *s++;
    *q = '\0';
  }
  return p;
}

/* ********************************************************************** */

char   *
pstrcat(s1, s2)
  char   *s1, *s2;
{
  register char *p = s1;

  while (*p)
    p++;
  while (*s2)
    *p++ = *s2++;
  *p = '\0';

  return s1;
}

/* ********************************************************************** */

/* return src of char in charset. charset is a string */
uChar 
ch_in_set(c, s)
  uShrt   c;
  char   *s;
{
  char    cc;
  register char a;
  register uChar i;

  cc = (char) c;
  for (i = 0; ((a = s[i]) != '\0') && (a != cc); i++);
  return (i);
}

/* ********************************************************************** */

/*
 * return src of str in charset. charset is a string set. case is significant
 */
uChar 
str_in_set(str, s)
  char   *str, *s[];
{
  register uChar i;
  for (i = 0; *s[i] && strcmp(str, s[i]); i++);
  return (i);
}

/* ********************************************************************** */

/* return next char which is non-whitespace char */
uChar 
skip_space(str, i)
  char   *str;
  uShrt   i;
{
  register uChar idx;
  idx = (uChar) i;
  while (isspace(str[idx]))
    idx++;
  return (idx);
}

/* ********************************************************************** */

void 
to_upper(str)
  char   *str;
{
  while ((*str = toupper_(*str)) != '\0')
    str++;
}

/* ********************************************************************** */

/* Get token which depends on the first letter. token need to be allocated. */
int 
get_token(str, curIndex, token)
  char   *str, *token;
  uChar  *curIndex;
{
  register uChar src, dst = 0;
  register int ch;                /* int for ctype compliance */
  int     tokenType;

  src = skip_space(str, (uShrt) * curIndex);

  if (str[src])
    if (isdigit(ch = str[src])) {        /* Grab the whole digit */
      while (isdigit(str[src]))
        token[dst++] = str[src++];
      tokenType = NUMBTOKEN;
    }
  /* Grab the whole identifier. There would be special treatment to modifiers */
    else if (is_alpha(ch)) {
      for (; ((ch = str[src]) != '\0') && (is_alpha(ch) || isdigit(ch));)
        token[dst++] = str[src++];
      tokenType = CHARTOKEN;
    }
  /*
   * The following would accept only one single char. The order should
   * reflect the frequency of the symbols being used
   */
    else {
      /* Is operator symbol ? */
      if (is_expr(ch))
        tokenType = EXPRTOKEN;
      /* Is addressing mode symbol ? */
      else if (is_addr(ch))
        tokenType = ADDRTOKEN;
      /* Is concatenation symbol ? */
      /* currently force so that there is no double '&' */
      else if (ch == cat_sym)
        if (str[src + 1] == '&') {
          token[dst++] = str[src++];
          tokenType = EXPRTOKEN;
        } else
          tokenType = APNDTOKEN;
      /* comment symbol ? */
      else if (ch == com_sym)
        tokenType = COMMTOKEN;
      /* field separator symbol ? */
      else if (ch == sep_sym)
        tokenType = FSEPTOKEN;
      /* modifier symbol ? */
      else if (ch == mod_sym)
        tokenType = MODFTOKEN;
      else if ((ch == '|') && (str[src + 1] == '|')) {
        token[dst++] = str[src++];
        tokenType = EXPRTOKEN;
      } else
        tokenType = MISCTOKEN;

      token[dst++] = str[src++];
    }
  else
    tokenType = NONE;

  token[dst] = '\0';
  *curIndex = src;

  return (tokenType);
}
