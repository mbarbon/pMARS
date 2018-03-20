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
 * pos.c: RNG and positioning functions
 * $Id: pos.c,v 1.3 1996/02/20 19:15:46 stst Exp stst $
 */
#include "global.h"
#include "sim.h"

#ifdef NEW_STYLE
int     posit(void);
void    npos(void);
S32_T   rng(S32_T seed);
#endif

/* minimal standard random number generator; integer version 2
 * Communications of the ACM, 31:10 (1988)
 * returns 1 <= seed <= 2^31-2, cycle: 2^32, tested and approved */
S32_T
rng(seed)
  S32_T   seed;
{
  register S32_T temp = seed;
  temp = 16807 * (temp % 127773) - 2836 * (temp / 127773);
  if (temp < 0)
    temp += 2147483647;
  return temp;
}

#define RETRIES1 20                /* how many times to try generating one
                                 * position */
#define RETRIES2 4                /* how many times to start backtracking */
int
posit()
{
  int     pos = 1, i, retries1 = RETRIES1, retries2 = RETRIES2;
  int     diff;

  do {
    /* generate */
    warrior[pos].position =
      ((seed = rng(seed)) % (coreSize - 2 * separation + 1)) + separation;
    /* test for overlap */
    for (i = 1; i < pos; ++i) {
      /* calculate positive difference */
      diff = (int) warrior[pos].position - warrior[i].position;
      if (diff < 0)
        diff = -diff;
      if (diff < separation)
        break;                        /* overlap! */
    }
    if (i == pos)                /* no overlap, generate next number */
      ++pos;
    else {                        /* overlap */
      if (!retries2)
        return 1;                /* exceeded attempts, fail */
      if (!retries1) {                /* backtrack: generate new sequence starting
                                 * at an */
        pos = i;                /* arbitrary position (last conflict) */
        --retries2;
        retries1 = RETRIES1;
      } else                        /* generate new current number (pos not
                                 * incremented) */
        --retries1;
    }
  } while (pos < warriors);
  return 0;
}

void
npos()
{
  int     i, j;
  unsigned int temp;
  unsigned int room = coreSize - separation * warriors + 1;
  for (i = 1; i < warriors; i++) {
    temp = (seed = rng(seed)) % room;
    for (j = i - 1; j > 0; j--) {
      if (temp > warrior[j].position)
        break;
      warrior[j + 1].position = warrior[j].position;
    }
    warrior[j + 1].position = temp;
  }
  temp = separation;
  for (i = 1; i < warriors; i++) {
    warrior[i].position += temp;
    temp += separation;
  }
  for (i = 1; i < warriors; i++) {
    j = (seed = rng(seed)) % (warriors - i) + i;
    temp = warrior[j].position;
    warrior[j].position = warrior[i].position;
    warrior[i].position = temp;
  }
}
