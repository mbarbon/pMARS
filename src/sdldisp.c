/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1995 Albert Ma, Na'ndor Sieben, Stefan Strack, Mintardjo Wangsawidjaja and Martin Maierhofer
 * Copyright (C) 2003 M Joonas Pihlaja
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "SDL.h"

#define USE_BLIT_FONTS 1


/* ------------------------------------------------------------------------
 * Types and globals.
 */

/*-- Externs --*/
extern char *CDB_PROMPT;
extern int curPanel;			/* # current cdb panel: 1 or 2. */
extern int curAddr;			/* current core address in cdb. */

extern void sighandler(int dummy);	/* Used to tell cdb that the user
					   has interrupted a fight. */
/* Strings. */
extern char *outOfMemory;
extern char *errDisplayOpen;
extern char *failedSDLInit;
extern char *badModeString;
extern char *pressAnyKey;
extern char *errSpriteConv;
extern char *errSpriteColKey;
extern char *errSpriteSurf;
extern char *errSpriteFill;
extern char *errorHeader;


/*-- Colours --*/
typedef struct RGBColour_st {
    Uint8 r,g,b;
} RGBColour;

#define NCOLOURS	16
#define L	205			/* low intensity */
#define G	211			/* light grey intensity */
#define D	190			/* grey intensity */
#define H	255			/* high intensity */
static const RGBColour PaletteRGB[NCOLOURS] = {
#define BLACK 		0
    { 0, 0, 0 },			/* black */
    { 0, 0, L },			/* blue3 */
    { 0, L, 0 },			/* green3 */
    { 0, L, L },			/* cyan3 */
#define RED		4
    { L, 0, 0 },			/* red3 */
    { L, 0, L },			/* magenta3 */
    { L, L, 0 },			/* yellow3 */
#define LIGHTGREY	7
    { G, G, G },			/* light grey */
#define GREY            8
    { D, D, D },			/* grey */
    { 0, 0, H },			/* blue1 */
#define GREEN		10
    { 0, H, 0 },			/* green1 */
    { 0, H, H },			/* cyan1 */
#define LIGHTRED	12
    { H, 0, 0 },			/* red1 */
    { H, 0, H },			/* magenta1 */
#define YELLOW		14
    { H, H, 0 },			/* yellow1 */
#define WHITE 15
    { H, H, H }				/* white */
};
#undef L
#undef G
#undef D
#undef H

static Uint32 Colours[NCOLOURS];	/* Display format colours */
					/* that need to be converted. */

/*-- The display --*/

SDL_Surface *TheSurf;			/* The display surface. */

typedef struct VMode_st {		/* Video mode info */
    Uint16 w;				/* width */
    Uint16 h;				/* height */
    Uint16 bpp;				/* bits per pixel */
    Uint32 flags;			/* flags used to open the surface. */
} VMode;

VMode TheVMode;				/* Current video mode. */

#define NUM_VMODE_FLAGS 5
#define DEFAULT_VMODE_FLAGS (SDL_RESIZABLE | SDL_SWSURFACE);
#define DEFAULT_VMODE_W 640
#define DEFAULT_VMODE_H 480
#define DEFAULT_VMODE_BPP 0
static struct {
    const char *name;			/* full name of option */
    size_t siglen;			/* # significant chars */
    Uint32 value;			/* bitmask of flags to set or clear. */
} const VMode_flags[NUM_VMODE_FLAGS] = {
    { "fullscreen",	1, SDL_FULLSCREEN },
    { "resizable",	1, SDL_RESIZABLE },
    { "any",		1, SDL_ANYFORMAT },
    { "noframe",	1, SDL_NOFRAME },
    { "db",		1, SDL_DOUBLEBUF }
};


/*-- Font --*/

/* Font data is held in a NUMCHARS*HEIGHT byte bitmap, where the
 * bitmap for the Ith character consists of HEIGHT consecutive bytes
 * at index I*HEIGHT.  The HEIGHT bytes form a 8 by HEIGHT sized
 * bitmap, where the top left bit of the bitmap is the most
 * significant bit of the first byte.  For example, the bitmap
 * for the character `L' in an 8 by 8 font might be:
 *
 * byte #\bit # 7 6 5 4 3 2 1 0
 *   0		0 0 0 0 0 0 0 0
 *   1		0 1 0 0 0 0 0 0
 *   2		0 1 0 0 0 0 0 0
 *   3  	0 1 0 0 0 0 0 0
 *   4  	0 1 0 0 0 0 0 0
 *   5  	0 1 0 0 0 0 0 0
 *   6  	0 1 1 1 1 1 1 0
 *   7  	0 0 0 0 0 0 0 0
 *
 * { 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7e, 0x00 }
 * */
typedef struct FixedFontInfo_st {
    const Uint8 *bitmaps;		/* glyph bitmaps */
    Uint16 h;				/* height */
    Uint16 w;				/* width, always = 8. */
    Uint16 nchars;			/* # chars */
    SDL_Surface *glyphs[NCOLOURS];	/* glyphs bitmaps in native format. */
} FixedFontInfo;

/* Compile in a default font to use so the user doesn't have to lug around
 * a font file.  TODO: Let the user choose their own font somehow?
 **/
#include "fnt16.c"			/* Font bitmaps */
static FixedFontInfo CurrentFont;


/*-- Generic borders --*/

typedef struct Border_st {
    Sint16 lft, rgt, top, bot;
    /* Width's of borders on the left, right, top and bottom edges.  If
       negative, no visible border is drawn on the respective edge(s). */
    int colix;				/* Border colour index. */
} Border;

static const Border noborder = { 0, 0, 0, 0,  BLACK };


/*-- Panels --*/
typedef struct Panel_st {
    SDL_Rect	r;			/* Rectangle of this panel in
                                           display.  Includes border,
					   if the panel has one. */
    SDL_Rect	v;			/* The visual rectangle (R sans B).*/
    Border	b;			/* The border. */
} Panel;


/*-- Generic Panel Layouts --*/

typedef int (*layout_func)(Panel *p, void *parms);

typedef struct Layout_st {
    Panel p;				/* Panel for this layout. */
    Uint16 minw, maxw;			/* Bounds for width and height. */
    Uint16 minh, maxh;

    struct Layout_st *parent;		/* Pair layout that contains */
					/* this layout, or NULL for a */
					/* root layout. */

    /* The next fields specify call-backs for the user's layouts.  In
     * the tree of layouts, all callbacks are called in postorder
     * unless otherwise noted.  */
    void *parms;			/* Parameters passed to the
                                           all callbacks. */

    int (*before_layout)(struct Layout_st *, Uint16, Uint16);
	/* Called before proposing a layout with the new width and
	   height of the display so that panels may adjust their
	   minimum/maximum sizes.  Returns 1: ok to layout, 0: layout
	   will fail (display too small). */

    layout_func after_layout;
        /* Called after the sizes and positions of layed out panels
           have been decided, to give a chance for the user to fail a
           given layout and do other processing.  Returns 1: layout ok,
           0: layout failed. */

    layout_func doclear;
        /* Called to clear the layout.  Return value ignored.  Called in
           preorder. */

    layout_func doredraw;
        /* Called when a panel needs to be redrawn.  Return value ignored.
           Called in preorder. */

    void (*doremode)(void *parms);
        /* Called after a successfully laying out all layouts and the
	   display mode has (possibly) changed.  */

    void (*doclose)(void *parms);
        /* Called to close a layout. */

    layout_func dorefresh;
        /* Called to refresh a layout.  Return value ignored. */

    /* The following fields are used only for pair layouts to control
     * layout splits. */
    struct Layout_st *a, *b;		/* children. */
    Uint32 size;			/* percentage of split for child b. */
    Uint16 method;			/* Method of split. */
} Layout;

/* When a layout is split we must specify how the new layout is placed in
   relation to the old layout. */
#define BELOWS 0
#define ABOVES 1
#define LEFTS  2
#define RIGHTS 3

#define is_horizontal_split(method) (((method) & 2) == 2)

#define UNBOUNDED 32767			/* Maximum value of any minimum or
					   maximum bound for the height or
					   width of a layout. */

/* The memory for layout structures is statically allocated, and we provide a
   simple layout allocator from an array of available layouts.  When in the
   free-list, layouts are linked together by they `a' field of the Layout
   structure.  */
#define MAXLAYOUTS 40
Layout *layout_free_list = NULL;
Layout all_layouts[MAXLAYOUTS];

/*-- Generic Text Output panels (used for all text panels.) --*/

typedef struct TextOutput_st {
    Layout *layout;
    Sint16 x, y;			/* Point coordinates. */
    SDL_Rect refresh;			/* Rectangle that needs refreshing. */
    SDL_Rect drawn;			/* Rectangle where stuff was drawn. */
    struct TextOutput_st *next;		/* Next free TextOutput in free list.*/
} TextOutput;


/* pMARS specific: There must be two more TextOutputs than text inputs
 * because both the status line and warrior names panels are TextOutputs. */
#define MAXTEXTOUTPUTS 22
static TextOutput all_textoutputs[MAXTEXTOUTPUTS];
static TextOutput *textoutput_free_list = NULL;


/*-- Generic Text Input panels (used for cdb text panels.) --*/

typedef struct TextInput_st {
    TextOutput *out;			/* The output panel. */
    Layout *layout;			/* layout of this text input. */
    int active;				/* set if accepting input. */
    int has_focus;			/* set if has focus. */
    int needs_prompting;		/* set if prompts on activation.*/
    char prompt[MAXALLCHAR];		/* prompt string. */
    char buf[MAXALLCHAR];		/* input buffer. */
    size_t len;				/* input length. */
    int colix;				/* colour of user input. */

    struct TextInput_st *next;		/* Next free TextInput in free-list. */
} TextInput;

/* TextInput structures are also statically allocated and provide a simple
 * free-list allocator. */
#define MAXTEXTINPUTS 10
TextInput all_textinputs[MAXTEXTINPUTS];
TextInput *textinput_free_list = NULL;


/*-- (pMARS specific) --*/

static Uint32 WarColourIx[MAXWARRIOR];	/* Colour index (into Colours[]) of */
static Uint32 DieColourIx[MAXWARRIOR];  /* warriors and the colours they die
                                           with.. */

/* What core cells look like. */
typedef struct CellInfo_st {
    int		box_size;		/* size of one "pixel" of a cell. */
    int		interbox_space;		/* # pixels between cell's boxes. */
    int		intercell_space;	/* # pixels between cells. */
} CellInfo;

/* The different cell sizes chosen by the pMARS display mode. */
static const CellInfo DesiredCells[10] = {
    {  1,  0,  2 },		/* 0 */
    {  1,  0,  2 },		/* 1 */
    {  1,  0,  3 },		/* 2 */
    {  2,  0,  2 },		/* 3 */
    {  3,  0,  1 },		/* 4 */
    {  3,  1,  1 },		/* 5 */
    {  4,  0,  1 },		/* 6 */
    {  5,  2,  3 },		/* 7 */
    { 10,  5,  8 },		/* 8 */
    { 20, 10, 16 },		/* 9 */
};

/* The arena (core). */
typedef struct Arena_st {
    Layout		*layout;	/* Layout used for a core. */
    unsigned char 	*core;		/* For each core cell, four colour
					   indices that say what the colour of
					   the cell boxes are in the top-left,
					   top-right, bottom-right, and
					   bottom-left */
    Uint32		coresize;	/* Guess what? */
    CellInfo 		cell;		/* The actual cell used that fits
					   the core display. */
    CellInfo		desired_cell;	/* The cell chosen by the current
					   pMARS display mode. */

    SDL_Surface		***blitboxes;
	/* For each used colour index (into Colours[]) we precompute sixteen
	   sprites, each representing a different way that the top-left,
	   top-right, bottom-left and bottom-right boxes of a 2x2 grid can be
	   filled with that colour.  During a fight these are then blitted to
	   the screen using a potentially fast blit. (For very small core
	   cells this is a lot of overhead, but it should pay off for larger
	   cells.)  This precomputation is done each time the doremode()
	   function of a core layout is called.  */
} Arena;

/* Bit masks of the top-left, top-right, bottom-left, and bottom-right boxes
   of a cell. */
#define TL 0x00000001
#define TR 0x00000002
#define BL 0x00000004
#define BR 0x00000008

static Layout *CoreLayout;		/* The Layout used for the core
                                           display. */
static Arena CoreArena;			/* The Arena used for the core
                                           display. */
int ArenaX, ArenaY;			/* The top-left coordinates of the
					   cell at address 0 in core. */
int CellSize;				/* # pixels used by a core cell. */
int CellsPerLine;			/* # cells that fit in one row. */
SDL_Surface ***BlitBoxes;		/* Core cell sprites (same array as in
					   CoreArena.blitboxes.) */

/* Macros to convert pixel offsets from (ArenaX,ArenaY) into core addresses
   and vice versa. */
#define xkoord(addr) (ArenaX + CellSize*((addr) % CellsPerLine))
#define ykoord(addr) (ArenaY + CellSize*((addr) / CellsPerLine))
#define addr_of(x,y) (  ((x)-ArenaX)/CellSize \
			+ CellsPerLine*((y)-ArenaY)/CellSize )


static Layout *StatusLayout = NULL;	/* The Layout used for the status
                                           line. */

static Layout *MetersLayout = NULL;	/* The Layout used for the process and
					   cycle meters. */

int MetersX;				/* X-coordinate of meters low point. */
int splY[MAXWARRIOR];			/* Y-coordinates of process meters. */
int cycleY;				/* Y-coordinate of cycle counter. */
int processRatio;			/* Ratio of processes to pixels. */
int cycleRatio;				/* Ratio of cycles to pixels. */
					/* Both ratios are at least 1. */

static TextInput *TextPanels[MAXTEXTINPUTS]; /* cdb input panels, indexed by
						curPanel-1. */
static int NTextPanels = 0;

/* These are filled in clparse.c */
#define SDLGR_NOPTIONS 1
int MaxSDLOptions;
const char sdlgr_Options[SDLGR_NOPTIONS] = {
    'm'					/* The new -m option is used to give
					 * the desired screen mode.  */
};
const char *sdlgr_Storage[SDLGR_NOPTIONS] = {
    NULL				/* Pointer to argument of -m option */
};

/* Command History. */

typedef struct Cmd_st {
    char *cmd;				/* The command. */
    struct Cmd_st *next;		/* earlier command in history. */
    struct Cmd_st *prev;		/* later command in history. */
} Cmd;

Cmd CmdRing;				/* List header. */

/* ------------------------------------------------------------------------
 * File-local prototypes.
 */

/* Utilities */
static void exit_panic (const char *msg, const char *reason);
static void xstrncpy (char *dst, const char *src, size_t maxbuf);
static void xstrncat (char *dst, const char *src, size_t maxbuf);
static void xstrnapp (char *dst, unsigned int c, size_t maxbuf);
static char *xstrdup (const char *s);

static void putpixel (SDL_Surface *s, int x, int y, Uint32 col);

static void clip_rect (const SDL_Rect *c, SDL_Rect *r);
static void join_rect (const SDL_Rect *r, SDL_Rect *c);
static void clear_rect (const SDL_Rect *r);

/* Display */
static const char* decode_vmode_string (VMode *m, const char *str);
static void Slock (SDL_Surface *screen);
static void Sunlock (SDL_Surface *screen);
static SDL_Surface * set_VMode (VMode* m);
/* static void clear_display (void); */
static void refresh_rect(const SDL_Rect *r);

/* Text output primitives. */
static void outblankxy (Sint16 x, Sint16 y, int colix, const SDL_Rect *clip);
static void outcharxy (unsigned char c, Sint16 x, Sint16 y, int colix,
		       const SDL_Rect *clip);
static void Font_Init(FixedFontInfo *f, const Uint8* bitmaps,
		      Uint16 w, Uint16 h, Uint16 nchars);

/* Borders. */
static Border border (Sint16 l, Sint16 r, Sint16 t, Sint16 b, int colix);
static void draw_border (const SDL_Rect *r, const Border *b);

/* Panels */
static Panel new_panel (Sint16 x, Sint16 y, Uint16 w, Uint16 h, Border b);
static void recompute_panel_view (Panel *p);
static const SDL_Rect *panel_view (const Panel *p);
static void refresh_panel (const Panel *p);
static void panel_text_size (const Panel *p, int *cols, int *lines);
static void clear_panel (Panel *p);

/* Layouts; general. */
static void init_all_layouts (void);
static int default_before_layoutfunc (Layout *a, Uint16 w, Uint16 h);
static int default_after_layoutfunc (Panel *p, void *parms);
static int default_redrawfunc (Panel *p, void *parms);
static int default_clearfunc (Panel *p, void *parms);
static int default_refreshfunc (Panel *p, void *parms);
static void default_closefunc (void *parms);
static void default_remodefunc (void *parms);

static Layout * alloc_layout (void);
static Layout * alloc_pair_layout (void);
static void free_layout (Layout *lay);
static Uint16 add_bounds (Uint32 a, Uint32 b);
/* static Uint16 sub_bounds (Uint32 a, Uint32 b); */
static void synthesize_bounds (Layout *pair);
static Layout * get_root_layout (Layout *a);
static void recursive_free_layout (Layout *a);
static Layout * get_sibling_layout (const Layout *a);
static void close_layout (Layout *a);
static void propose_layout (Layout *pair, Sint16 x, Sint16 y, Uint16 w, Uint16 h);
static void redraw_layout_borders (Layout *a);
static void clear_layout (Layout *a);
static int callback_before_layouts (Layout *a, Uint16 w, Uint16 h);
static int callback_after_layouts (Layout *a);
static void redraw_layout (Layout *a);
static void refresh_layout (Layout *a);
static int layout (Layout *root, Sint16 x, Sint16 y, Uint16 w, Uint16 h);

/* TextOutput layouts. */
static void free_text_output(TextOutput *t);
static void init_all_text_outputs (void);
static int doclear_textoutput (Panel *p, void *parms);
static void doclose_textoutput (void *parms);
static int dorefresh_textoutput (Panel *p, void *parms);
static TextOutput *alloc_text_output (Layout *a);
static void clear_to_eol (TextOutput *p);
static void cr (TextOutput *t);
static void emit (TextOutput *t, unsigned char c, int colix);
static void backspace (TextOutput *t);
static void backchar (TextOutput *t);
static void delchar (TextOutput *t);
static void type (TextOutput *t, const char *str, int colix);
static void type_nowrap (TextOutput *t, const char *str, int colix);

/* TextInput layouts. */
static void free_text_input (TextInput *t);
static void init_all_text_inputs (void);
static void cursoron (TextInput *t);
static void cursoroff (TextInput *t);
static int after_layout_textinput (Panel *p, void *parms);
static int doredraw_textinput (Panel *p, void *parms);
static void doclose_textinput (void *parms);
static int dorefresh_textinput (Panel *p, void *parms);
static TextInput * alloc_text_input (Layout *a);
static void activate (TextInput *t);
static void deactivate (TextInput *t);
static int is_active (TextInput *t);
static void give_focus (TextInput *t);
static void take_focus (TextInput *t);
static void set_prompt (TextInput *t, const char *prompt);
static void add_char (TextInput *t, char c);
static void del_char (TextInput *t);
static void reset_input (TextInput *t, const char *str);
static void finish_input (TextInput *t, char *buf, size_t maxbuf);
static void hide_input (TextInput *t);

/* Core Arena layout. */
static Uint32 get_cell_size (const CellInfo *c);
static CellInfo choose_cell (int mode);
static void display_box (int addr, Uint32 boxes, int colix);
static int decr_cell_size (CellInfo *c);
static int before_layout_arena (Layout *layout, Uint16 w, Uint16 h);
static int after_layout_arena (Panel *p, void *parms);
static void doclose_arena (void *parms);
static int doredraw_arena (Panel *p, void *parms);
static int doclear_arena (Panel *p, void *parms);
static void make_blitboxes_for_colix (Arena *a, int colix);
static void make_blitbox (Arena *a, int colix, Uint32 boxes);
static void fill_box (SDL_Surface *s, Sint16 x, Sint16 y, Uint16 w, Uint16 h, Uint32 col);
static void free_blitboxes(Arena *a);
static void doremode_arena (void *parms);
static void init_arena (Layout *layout, size_t coresize, int displaymode);

/* Status line, warrior names, process/cycles meters layouts. */
static int doredraw_status (Panel *p, void *parms);
static int doredraw_names (Panel *p, void *parms);
static int doredraw_meters (Panel *p, void *parms);
static int after_layout_meters (Panel *p, void *parms);

/* Pulling it all together. */
static void redraw (void);
static void relayout (Uint16 w, Uint16 h);
static void init_layout (void);

/* Cdb text panels. */
static int split_text_panel (int panel, int size, Uint16 method);
static void close_text_panel (int panel);

/* Events and keyboard input. */
static void default_handler (SDL_Event *e);
static char conv_key (const SDL_keysym *s);
static int macro_key (const SDL_keysym *s, char *buf, int maxbuf);

/* Command History */
static Cmd * get_cmd_ring();
static void add_cmd (const char *cmd);

/* Events & keyboard. */
static void default_handler (SDL_Event *e);
static int special_keyhandler (const SDL_keysym *s);
static char conv_key (const SDL_keysym *s);
static int macro_key (const SDL_keysym *s, char *buf, int maxbuf);

/* Misc. */
static void exit_hook (void);


/* ------------------------------------------------------------------------
 * Utilities
 */

#undef max
#undef min
#define max(a,b)  ((a) < (b) ? (b) : (a))
#define min(a,b)  ((a) < (b) ? (a) : (b))

/* Exit in a hurry. */
static void
exit_panic(const char *msg, const char *reason)
{
    fprintf(stderr, errorHeader, msg);
    if (reason) {
	fprintf(stderr, ": %s", reason);
    }
    fprintf(stderr, ".\n");
    Exit(1);
}

/* Safer strncpy() that always nil-terminates (except when maxbuf==0). */
static void
xstrncpy(char *dst, const char *src, size_t maxbuf)
{
    size_t k;
    if (maxbuf==0) { return; }
    for (k=0; k<maxbuf-1 && src[k]; k++) {
	dst[k] = src[k];
    }
    dst[k] = 0;
}

/* Saner strncat(). */
static void
xstrncat(char *dst, const char *src, size_t maxbuf)
{
    size_t len, k;
    if (maxbuf==0) { return; }
    len = strlen(dst);
    for (k=0; k+len+1<maxbuf && src[k]; k++) {
	dst[k+len] = src[k];
    }
    dst[k+len] = 0;
}

/* Append a character. */
static void
xstrnapp(char *dst, unsigned int c, size_t maxbuf)
{
    size_t len;
    if (maxbuf==0) { return; }
    len = strlen(dst);
    if (len+1 < maxbuf) {
	dst[len] = c;
    }
    dst[len+1] = 0;
}

/* Copy a string. */
static char *
xstrdup(const char *s)
{
    char *t;
    assert(s);
    t = MALLOC((strlen(s)+1)*sizeof(char));
    if (t==NULL) {
	exit_panic(outOfMemory, NULL);
    }
    { char *q = t;  while ((*q++ = *s++)) {} }
    return t;
}

/* Compute the clip rectangle R \cap C */
static void
clip_rect(const SDL_Rect *c, SDL_Rect *r)
{
    Sint16 x1,y1;
    Sint16 x0,y0;
    x0 = max(r->x, c->x);
    y0 = max(r->y, c->y);
    x1 = min(r->x+r->w, c->x+c->w);
    y1 = min(r->y+r->h, c->y+c->h);
    r->x = x0;
    r->y = y0;
    r->w = max(x1-x0,0);
    r->h = max(y1-y0,0);
}

/* C := smallest rectangle containing both R and C. */
static void
join_rect(const SDL_Rect *r, SDL_Rect *c)
{
    if (c->w && c->h) {
	Sint16 x0,y0;
	Sint16 x1,y1;
	x0 = min(r->x, c->x);
	y0 = min(r->y, c->y);
	x1 = max(r->x+r->w, c->x+c->w);
	y1 = max(r->y+r->h, c->y+c->h);
	c->x = x0;
	c->y = y0;
	c->w = x1-x0;
	c->h = y1-y0;
    } else {
	*c = *r;
    }
}


/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 * This is from the SDL docs, so it can't be all that bad.
 */
static void
putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

/* ------------------------------------------------------------------------
 * Display surface & colour management
 *
 * Globals:
 *  SDL_Surface *TheSurf; (read/write)
 *     Display surface.
 *
 *  VMode TheVMode;
 *     Global video mode.
 *
 *  NCOLOURS (= 16) (read-only)
 *	The number of colours in _our_ palette.  pMARS uses only the 16
 *	standard VGA colours for a uniform look-and-feel. :)
 *
 *  static const RGBColour PaletteRGB[NCOLOURS]; (read-only)
 *	RGBValues of _our_ palette.  Indexed by <colourname> #defines:
 *
 *  Uint32 Colours[NCOLOURS]; (read-only)
 *	The colours in PaletteRGB[] in the format of the display surface.
 *	These must be initialised using SDL_MapRGB() before use.obs
 *
 * Functions:
 *  const char *decode_vmode_string(VMode *mode, const char* argstr);
 *	Decodes a videomode string ARGSTR and places the results in
 *	the video mode info structure MODE.  Returns NULL on success.
 *	On failure, returns a pointer to the trailing substring that
 *	wasn't understood.
 *
 *  SDL_Surface *set_VMode(VMode* mode)
 *	Attempt to set the display video mode to the given MODE and return
 *	a pointer to the opened surface --- NULL on failure.  On success
 *	the flags of MODE are set to indicate the actual flags of the surface,
 *	and the colours from RGBColours are mapped into display-native
 *	colours into Colours[].
 *
 *  void Slock(SDL_Surface *);
 *  void Sunlock(SDL_Surface *);
 *	Lock/Unlock the given surface if necessary.
 */


/* Decode VMode options from a VMode string.  On success, the given
 * VMode structure contains the desired mode, and the function returns
 * NULL.  Otherwise the function returns a non-NULL pointer to a
 * trailing substring of the mode string that doesn't make sense.
 * The option string must have this format:
 *	[<width>x<height>][@<bpp>][:<flag1>[:<flag2>[...[:<flagN>]...]]]
 * The flags may be preceded by a '-' to indicate that the given
 * flag must be cleared.  Valid examples:
 *
 *	640x480				640 by 480 at any bpp, window.
 *	640x480x16			640 by 480 at 8 bits per pixel window.
 *	320x200x16:noframe		320 by 200 window without a frame.
 *	:fullscreen			default width/height, fullscreen.
 */
static const char*
decode_vmode_string(VMode *m, const char *str)
{
    int tmp;
    int set_dims = 0;
    /* Decode width, height, bpp in the format <WIDTH>x<HEIGHT>[@<BPP>].
     * bpp==0 means that any pixel format is acceptable (most probably the
     * current pixel format of the display device).
     */
    m->w = DEFAULT_VMODE_W;
    m->h = DEFAULT_VMODE_H;
    m->bpp = DEFAULT_VMODE_BPP;
    m->flags = DEFAULT_VMODE_FLAGS;
    if (1==sscanf(str, "%d", &tmp)) {
	if (3!=sscanf(str,"%hux%hu@%hu", &m->w, &m->h, &m->bpp)) {
	    m->w = DEFAULT_VMODE_W;
	    m->h = DEFAULT_VMODE_H;
	    m->bpp = DEFAULT_VMODE_BPP;
	    if (2!=sscanf(str, "%hux%hu", &m->w, &m->h)) {
		return str;
	    }
	}
	set_dims = 1;
	while (*str && *str!=':') { str++; } /* skip to ':' */
    } else if (*str=='@') {
	if (1!=sscanf(str,"@%hu", &m->bpp)) {
	    m->bpp = DEFAULT_VMODE_BPP;
	    return str;
	}
	while (*str && *str!=':') { str++; } /* skip to ':' */
    }
    if (*str==':') { str++; }		/* skip over ':' */

    /* Decode flags separated by ':' and optionally preceded by '-'. */
    if (m->bpp == 0) { m->flags |= SDL_ANYFORMAT; }

    while (*str) {
	int i;
	int set_them = 1;
	if (str[0]=='-') { set_them = 0; str++; }

	for (i=0; i<NUM_VMODE_FLAGS; i++) {
	    if (strlen(str)>=VMode_flags[i].siglen) {
		if (0==strncmp(str, VMode_flags[i].name,
			       VMode_flags[i].siglen))
		{
		    if (set_them) {
			m->flags |= VMode_flags[i].value;
		    } else {
			m->flags &= ~VMode_flags[i].value;
		    }
		    break;
		}
	    }
	}
	if (i==NUM_VMODE_FLAGS) {
	    return str;		/* unknown flag */
	}
	while (*str && *str!=':') { str++; } /* skip to ':' */
	if (*str) { str++; }	/* skip over ':' */
    }

    /* If the user didn't set the dimensions and requested fullscreen mode,
     * we should find the biggest fullscreen mode available and use that.
     * In case no modes are available, or "all" modes are ok, use the defaults.
     */
    if (!set_dims && ((m->flags & SDL_FULLSCREEN)==SDL_FULLSCREEN)) {
	SDL_Rect **modes;
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
	if (modes == NULL || modes==(SDL_Rect**)(-1)) {
	    /* Use defaults that are set already. */
	} else {
	    /* Find the biggest fullscreen mode. */
	    int i, bigix = -1;
	    double bigsize = 0;
	    for (i=0; modes[i]; i++) {
		double size = (double)modes[i]->w*modes[i]->h;
		if (size>bigsize) {
		    bigsize = size;
		    bigix = i;
		}
	    }
	    if (bigix>=0) {
		m->w = modes[bigix]->w;
		m->h = modes[bigix]->h;
	    }
	}
    }

    return 0;				/* all ok. */
}

/* Lock a surface if necessary. */
static void
Slock(SDL_Surface *screen)
{
    if ( SDL_MUSTLOCK(screen) )
    {
	while (SDL_LockSurface(screen)<0) {
	    SDL_Delay(50);		/* wait for a while. */
	}
    }
}

/* Unlock a surface if necessary. */
static void
Sunlock(SDL_Surface *screen)
{
    if ( SDL_MUSTLOCK(screen) )
    {
	SDL_UnlockSurface(screen);
    }
}

/* Attempt to initialise the display to a given video mode M.  Returns
 * the surface structure of the display if successful and modifies the
 * VMode structure M to reflect the actual flags of the surface.  On
 * failure returns NULL.  On success, remaps the colours in Colours[] to
 * the display native format as a side-effect.
 **/
static SDL_Surface *
set_VMode(VMode* m)
{
    SDL_Surface *surf = NULL;
    Uint32 flags = m->flags;

    if ((flags & SDL_FULLSCREEN) == SDL_FULLSCREEN) { /* only in fullscreen. */
	const SDL_VideoInfo *info = SDL_GetVideoInfo();
	if ( info->blit_fill ) {
	    /* We want accelerated blitting. */
	    flags |= SDL_HWSURFACE;
	}
	if ((flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
	    /* Direct hardware blitting without double-buffering
	     * causes really bad flickering.
	     */
	    if ( m->bpp>0
		 && info->video_mem*1024 > (Uint32)(m->h*m->w*m->bpp/8) )
	    {
		flags |= SDL_DOUBLEBUF;
	    } else {
		flags &= ~SDL_HWSURFACE;
	    }
	}
	if ((flags & SDL_HWSURFACE) == SDL_HWSURFACE) {
	    flags &=~ SDL_ANYFORMAT;
	}
    }

    if (!(surf = SDL_SetVideoMode(m->w, m->h, m->bpp, flags))
	&& flags != m->flags)
    {
	flags = m->flags;		/* try with original flags. */
	surf = SDL_SetVideoMode(m->w, m->h, m->bpp, flags);
    }
    m->flags = flags;

    /* Update VMode with the one we actually got. */
    TheSurf = surf;
    if (surf) {
	m->flags = surf->flags;
	m->w = surf->w;
	m->h = surf->h;
	m->bpp = surf->format->BitsPerPixel;

	/* Set the caption on our window.
	 */
	SDL_WM_SetCaption("pMARS", "pMARS");
#if 0
	printf("%dx%d@%d:%x\n", m->w, m->h, m->bpp, m->flags);
#endif

	/* Map the colours we will use into display-native format. */
	{
	    int k;
	    for (k=0; k<NCOLOURS; k++) {
		RGBColour c = PaletteRGB[k];
		Colours[k] = SDL_MapRGB(surf->format, c.r, c.g, c.b);
	    }
	}

	/* Initialise the font for the colours we will use. */
	Font_Init(&CurrentFont, fnt16_raw, 8, 16, 128);
    }
    return surf;
}


#if 0
static void
clear_display(void)
{
    SDL_Rect r;
    r.x = 0;
    r.y = 0;
    r.w = TheSurf->w;
    r.h = TheSurf->h;
    Slock(TheSurf);
    SDL_FillRect(TheSurf, &r, Colours[BLACK]);
    Sunlock(TheSurf);
}
#endif


static void
refresh_rect(const SDL_Rect *r)
{
    SDL_UpdateRect(TheSurf, r->x, r->y, r->w, r->h);
}

/* ------------------------------------------------------------------------
 * Text and font routines.
 *
 * Globals:
 *   struct FixedFontInfo_st CurrentFont; (read-only)
 *
 * Functions:
 *  void outcharxy(unsigned char c, Sint16 x, Sint16 y, Uint32 col,
 *		   const SDL_Rect *clip);
 *	Draw the glyph for character C in the current font in the
 *	given (display format) colour COL into the global display
 *	surface TheSurf.  Clip to the rectangle CLIP if it is non-NULL.
 *
 *   void outblankxy(Sint16 x, Sint16 y, Uint32 col, const SDL_Rect *clip);
 *	Fill an 8 by HEIGHT (of the current font) rectangle whose top-left
 *	coordinate is (X,Y) with the given colour.  Clip to CLIP is non-NULL.
 *
 *   void Font_Init(FixedFontInfo *f, const Uint8 *bitmaps,
 *		Uint16 w, Uint16 h, Uint16 nchars);
 *	Initialise the font into display native format.
 **/

static void
Font_Init(FixedFontInfo *f, const Uint8* bitmaps, Uint16 w, Uint16 h, Uint16 nchars)
{
    int i;
    Uint32 key;
    f->bitmaps = bitmaps;
    f->w = w;
    f->h = h;
    f->nchars = nchars;

    /* If bitmaps == NULL, then this is the first call to initialise a
     * font and we should load all the display native format surfaces
     * with NULL so deallocation works when it is properly initialised. */
    if (bitmaps==NULL) {
	for (i=0; i<NCOLOURS; i++) {
	    f->glyphs[i] = NULL;	/* clear glyphs for all colours. */
	}
	return;
    }
#if USE_BLIT_FONTS
    /* Free the earlier display native format glyphs. */
    for (i=0; i<NCOLOURS; i++) {
	if (f->glyphs[i]) {
	    SDL_FreeSurface(f->glyphs[i]);
	    f->glyphs[i] = NULL;
	}
    }

    /* Create the new glyphs for each colour. */
    for (i=0; i<NCOLOURS; i++) {
	int c;
	Uint32 col;
	SDL_Surface *s;
	s = SDL_CreateRGBSurface(SDL_ANYFORMAT, w*nchars, h,
				 TheSurf->format->BitsPerPixel,
				 TheSurf->format->Rmask,
				 TheSurf->format->Gmask,
				 TheSurf->format->Bmask,
				 0);	/* no alpha. */
	if (s == NULL) {
	    exit_panic(errSpriteSurf, SDL_GetError());
	}

	/* Set colorkey (=black) for this glyphmap. */
	key = SDL_MapRGB(s->format, 0, 0, 0); /* colorkey */
	if (-1==SDL_SetColorKey(s, SDL_RLEACCEL, key)) {
	    exit_panic(errSpriteColKey, SDL_GetError());
	}

	/* Get the foreground colour and clear the background to the
	 * colorkey. */
	col = SDL_MapRGB(s->format, PaletteRGB[i].r,
			 PaletteRGB[i].g, PaletteRGB[i].b);
	fill_box(s, 0, 0, c*w*h, h, key);

	/* Draw the individual characters (vewy slowly :/) */
	Slock(s);
	for (c=0; c<nchars; c++) {
	    Sint16 x, y;
	    for (y=0; y<h; y++) {
		for (x=0; x<w; x++) {
		    /* Get a bit from the bitmap.  The MSB in a byte
		     * is the leftmost. */
		    Uint32 bitno = c*w*h + y*w + x;
		    Uint32 ix = bitno/8;
		    Uint8 row = f->bitmaps[ix];
		    bitno = 7 - (bitno & 7);
		    if (row & (1<<bitno)) {
			putpixel(s, c*w+x, y, col);
		    }
		}
	    }
	}
	Sunlock(s);

	/* Convert glyphs to video format. */
	f->glyphs[i] = SDL_DisplayFormat(s);
	SDL_FreeSurface(s);
	if (f->glyphs[i] == NULL) {
	    exit_panic(errSpriteConv, SDL_GetError());
	}
    }
#endif
}

/* Clear a character sized area. */
static void
outblankxy(Sint16 x, Sint16 y, int colix, const SDL_Rect *clip)
{
    SDL_Rect r;
    r.x = x; r.y = y;
    r.w = CurrentFont.w;
    r.h = CurrentFont.h;
    if (!clip) {
	SDL_Rect c;
	c.x = c.y = 0;
	c.w = TheSurf->w; c.h = TheSurf->h;
	clip_rect(&c, &r);
    } else {
	clip_rect(clip, &r);
    }
    Slock(TheSurf);
    SDL_FillRect(TheSurf, &r, Colours[colix]);
    Sunlock(TheSurf);
}

/* Draw a character at a position. */
static void
outcharxy(unsigned char c, Sint16 x, Sint16 y, int colix,
	  const SDL_Rect *clip)
{
    SDL_Rect oldclip;
    SDL_Rect src, dst;
#if USE_BLIT_FONTS
    if (c >= CurrentFont.nchars) { return; }
    src.h = dst.h = CurrentFont.h;
    src.w = dst.w = CurrentFont.w;
    dst.x = x; dst.y = y;
    src.x = c*CurrentFont.w;
    src.y = 0;
    SDL_GetClipRect(TheSurf, &oldclip);
    SDL_SetClipRect(TheSurf, clip);

    SDL_BlitSurface(CurrentFont.glyphs[colix], &src, TheSurf, &dst);

    SDL_SetClipRect(TheSurf, &oldclip);
#else
    SDL_Rect r;
    int i,j,h;
    if (c >= CurrentFont.nchars) { return; }

    if (!clip) {
	r.x = r.y = 0;
	r.w = TheSurf->w;
	r.h = TheSurf->h;
    } else {
	r = *clip;
    }
    outblankxy(x,y, BLACK, &r);
    if (c==' ') { return; }

    h = CurrentFont.h;
    assert(CurrentFont.w == 8);

    /* Draw character pixel by pixel.  Replace with something faster
     * if it's too slow. */
    Slock(TheSurf);
    for (j=r.y; j<r.y+r.h; j++) {
	Uint8 row = CurrentFont.bitmaps[c*h+j-y];
	for (i=r.x; i<r.x+r.w; i++) {
	    if (row & (1<<(7-(i-x)))) {
		putpixel(TheSurf, i, j, Colours[colix]);
	    }
	}
    }
    Sunlock(TheSurf);
#endif /* USE_BLIT_FONTS */
}


/* ------------------------------------------------------------------------
 * Panels
 *
 * Panels are rectangular areas on the display surface that may have a border.
 **/

/* Create a new border. */
static Border
border(Sint16 l, Sint16 r, Sint16 t, Sint16 b, int colix) {
    Border e;
    e.lft = l; e.rgt = r; e.top = t; e.bot = b; e.colix = colix;
    return e;
}

/* Draw a border. */
static void
draw_border(const SDL_Rect *r, const Border *b)
{
    SDL_Rect q;
    if (b->lft > 0 && r->h>0) {
	q = *r; q.w = 1;
	SDL_FillRect(TheSurf, &q, Colours[b->colix]);
    }
    if (b->rgt > 0 && r->w>0 && r->h>0) {
	q = *r; q.w = 1; q.x += r->w-1;
	SDL_FillRect(TheSurf, &q, Colours[b->colix]);
    }
    if (b->top > 0 && r->w>0) {
	q = *r; q.h = 1;
	SDL_FillRect(TheSurf, &q, Colours[b->colix]);
    }
    if (b->bot > 0 && r->h>0 && r->w>0) {
	q = *r; q.h = 1; q.y += r->h-1;
	SDL_FillRect(TheSurf, &q, Colours[b->colix]);
    }
}

/* Set the view rectangle of a panel. */
static void
recompute_panel_view(Panel *p)
{
    p->v = p->r;
    p->v.x += abs(p->b.lft);
    p->v.y += abs(p->b.bot);
    p->v.w = 0;
    if (p->r.w > abs(p->b.lft) + abs(p->b.rgt)) {
	p->v.w = p->r.w - (abs(p->b.lft) + abs(p->b.rgt));
    }
    p->v.h = 0;
    if (p->r.h > abs(p->b.top) + abs(p->b.bot)) {
	p->v.h = p->r.h - (abs(p->b.top) + abs(p->b.bot));
    }
}

/* Create a new panel. */
static Panel
new_panel(Sint16 x, Sint16 y, Uint16 w, Uint16 h, Border b)
{
    Panel p;
    p.r.x = x;
    p.r.y = y;
    p.r.w = w;
    p.r.h = h;
    p.b = b;
    recompute_panel_view(&p);
    return p;
}

/* Return the "view rectangle" that contains the contents of the panel. */
static const SDL_Rect*
panel_view(const Panel *p)
{
    return &p->v;
}

/* Make sure the panel area is updated on the screen. */
static void
refresh_panel(const Panel *p)
{
    refresh_rect(&p->r);
}

/* Return the size of the panel's view rectangle in columns and rows
 * of the current font.  */
static void
panel_text_size(const Panel *p, int *cols, int *lines)
{
    const SDL_Rect *r = panel_view(p);
    if (cols) { *cols = r->w / CurrentFont.w; }
    if (lines) { *lines = r->h / CurrentFont.h; }
}

/* Clear a rectangle. */
static void
clear_rect(const SDL_Rect *r)
{
    SDL_Rect c = *r;
    Slock(TheSurf);
    SDL_FillRect(TheSurf, &c, Colours[BLACK]);
    Sunlock(TheSurf);
}


/* Clear the contents of the panel and draw the border. */
static void
clear_panel(Panel *p)
{
    SDL_Rect r = p->r;
    SDL_FillRect(TheSurf, &r, Colours[BLACK]);
    draw_border(&r, &p->b);
}


/* ------------------------------------------------------------------------
 * Generic panel layout (with ideas cheerfully stolen from Andrew Plotkin's
 * GLK model.)
 *
 * Panel are isolated entities and don't know how to react to resizing and
 * other layout affecting events.  "Layout" structures link panels together
 * so that they may be layed out in simple ways.  New layouts are created
 * by splitting previously defined layouts, in which case the new layout
 * takes some part of the space of the splitted layout.
 *
 * When a layout is split, it is "replaced" by an internally generated
 * layout that has the old layout and the new layout as it's children.  By
 * splitting layouts like this, they form a binary tree with the leaf nodes
 * corresponding to actual panels that are drawn onto, and the internal
 * nodes to internally generated layout structures (called pair layouts.)
 **/

/* Initialise the statically allocated pool of layout structures. */
static void
init_all_layouts(void)
{
    static int done_init = 0;
    if (!done_init) {
	int i;
	memset((void*)all_layouts, 0, MAXLAYOUTS*sizeof(Layout));
	layout_free_list = &all_layouts[0];
	for (i=0; i<MAXLAYOUTS-1; i++) {
	    all_layouts[i].a = &all_layouts[i+1];
	}
	all_layouts[MAXLAYOUTS-1].a = NULL;
	done_init = 1;
    }
}

/* Default action to take before laying out a layout into a display
 * of size w by h pixels. */
static int
default_before_layoutfunc(Layout *a, Uint16 w, Uint16 h)
{
    a = a; w = w; h = h;
    return 1;
}

/* Default action to take after laying out. */
static int
default_after_layoutfunc(Panel *p, void *parms)
{
    parms = parms; p = p;
    return 1;
}

/* Default redraw only draws the borders of a panel. */
static int
default_redrawfunc(Panel *p, void *parms)
{
    parms = parms;
    draw_border(&p->r, &p->b);
    return 1;
}

/* Default clear only clears a panel (and possibly draws borders.) */
static int
default_clearfunc(Panel *p, void *parms)
{
    parms = parms;
    clear_panel(p);
    return 1;
}

/* Default refresh refreshes the panel. */
static int
default_refreshfunc(Panel *p, void *parms)
{
    parms = parms;
    refresh_panel(p);
    return 1;
}

/* Default action to take on closing a panel. */
static void
default_closefunc(void *parms)
{
    parms = parms;
}

/* Default action to take when the display mode has changed. */
static void
default_remodefunc(void *parms)
{
    parms = parms;
}

/* Allocate a new layout from the pool of statically allocated layouts. */
static Layout *
alloc_layout(void)
{
    Layout *lay;
    init_all_layouts();
    lay = layout_free_list;
    assert(lay && "Ran out of layouts.");
    layout_free_list = lay->a;

    lay->p = new_panel(0,0, 0,0, noborder);
    lay->a = lay->b = NULL;
    lay->minw = lay->minh = 0;
    lay->maxw = lay->maxh = UNBOUNDED;
    lay->before_layout = default_before_layoutfunc;
    lay->after_layout = default_after_layoutfunc;
    lay->doclear = default_clearfunc;
    lay->doredraw = default_redrawfunc;
    lay->doclose = default_closefunc;
    lay->doremode = default_remodefunc;
    lay->dorefresh = default_refreshfunc;
    lay->parms = NULL;
    lay->parent = NULL;
    lay->method = 0;
    lay->size = 0;
    return lay;
}

/* Allocate a pair-layout (internal layout). */
static Layout *
alloc_pair_layout(void)
{
    Layout *pair = alloc_layout();
    pair->before_layout = NULL;
    pair->after_layout = NULL;
    pair->doclear = NULL;
    pair->doredraw = NULL;
    pair->doclose = NULL;
    pair->doremode = NULL;
    pair->dorefresh = NULL;
    return pair;
}

/* Free a layout. */
static void
free_layout(Layout *lay)
{
    if (lay->doclose) { lay->doclose(lay->parms); }
    lay->a = layout_free_list;
    layout_free_list = lay;
}

/* Saturating addition of width|height bounds. */
static Uint16
add_bounds(Uint32 a, Uint32 b)
{
    Uint32 c = a + b;
    return (Uint16)(c <= UNBOUNDED ? c : UNBOUNDED);
}

#if 0
/* Saturating(?) subtraction of width|height bounds. */
static Uint16
sub_bounds(Uint32 a, Uint32 b)
{
    Sint32 c = a - b;
    if (a >= UNBOUNDED) { return UNBOUNDED; }
    return max(c, 0);
}
#endif

/* Compute the minimum and maximum sizes for all pair layouts bottom-up
 * from the bounds of the leaf layouts. */
static void
synthesize_bounds(Layout *pair)
{
    if (!pair->a && !pair->b) {
	assert(pair->minw <= pair->maxw);
	assert(pair->minh <= pair->maxh);
	return;				/* not a pair. */
    }
    assert(pair->a && pair->b);
    synthesize_bounds(pair->a);
    synthesize_bounds(pair->b);
    if (is_horizontal_split(pair->method)) {
	pair->minw = add_bounds(pair->a->minw, pair->b->minw);
	pair->maxw = add_bounds(pair->a->maxw, pair->b->maxw);
	pair->minh = max(pair->a->minh, pair->b->minh);
	pair->maxh = max(pair->a->maxh, pair->b->maxh);
    } else {
	pair->minh = add_bounds(pair->a->minh, pair->b->minh);
	pair->maxh = add_bounds(pair->a->maxh, pair->b->maxh);
	pair->minw = max(pair->a->minw, pair->b->minw);
	pair->maxw = max(pair->a->maxw, pair->b->maxw);
    }
    assert(pair->minw <= pair->maxw);
    assert(pair->minh <= pair->maxh);
}

/* Return a freshly allocated (default) layout with the given
 * minimum/maximum bounds and border. */
static Layout *
new_layout(Uint16 minw, Uint16 maxw,
	   Uint16 minh, Uint16 maxh,
	   Border b)
{
    Layout *a = alloc_layout();
    assert(minw <= maxw);
    assert(minh <= maxh);
    a->p.b = b;
    a->minw = add_bounds(minw, abs(b.lft) + abs(b.rgt));
    a->minh = add_bounds(minh, abs(b.top) + abs(b.bot));
    a->maxw = add_bounds(maxw, abs(b.lft) + abs(b.rgt));
    a->maxh = add_bounds(maxh, abs(b.top) + abs(b.bot));
    return a;
}

/* Return the root layout structure of a layout. */
static Layout *
get_root_layout(Layout *a)
{
    while (a->parent) { a = a->parent; }
    return a;
}

/* Free all contained layouts bottom-up of a layout, including itself. */
static void
recursive_free_layout(Layout *a)
{
    if (a->a) { recursive_free_layout(a->a); }
    if (a->b) { recursive_free_layout(a->b); }
    free_layout(a);
}

/* Returns the sibling layout of a non-root layout. */
static Layout *
get_sibling_layout(const Layout *a)
{
    if (a->parent == NULL) { return NULL; }
    if (a->parent->a != a) { return a->parent->a; }
    return a->parent->b;
}

/* Close a layout (and all child layouts).  There are three cases
 * depending on whether or not the layout's parent needs to be closed
 * too (layout A is the one we are closing):
 *
 * i)               ii)    O           iii)  G        G
 * A -> (nothing)         / \  -> B         / \  ->  / \
 *                       A   B             ?   O    ?   B
 *                                            / \
 *                                           A   B
 */
static void
close_layout(Layout *a)
{
    if (a->parent != NULL) {		/* Need to free a parent? */
	Layout *parent = a->parent;
	Layout *grandparent = parent->parent;
	Layout *b = get_sibling_layout(a);
	b->parent = grandparent;
	if (grandparent != NULL) {	/* Need to replace parent with b? */
	    if (grandparent->a == parent) {
		grandparent->a = b;
	    } else {
		assert(grandparent->b == parent);
		grandparent->b = b;
	    }
	}
	free_layout(parent);
    }
    recursive_free_layout(a);
}

/* Return a new layout that splits the given leaf-layout. */
static Layout *
split_layout(Layout *a,			/* layout to split */
	     Uint32 method,		/* split method */
	     Sint16 size,		/* size of split */
	     Uint16 mind, Uint16 maxd,
	     Border border)		/* border */
{
    Layout *b = alloc_layout();
    Layout *pair = alloc_pair_layout();
    Layout *parent;
    b->p.b = border;
    assert(!a->a && !a->b && "Can't split a pair layout.");
    assert(mind <= maxd);
    assert(a->minw <= a->maxw);
    assert(a->minh <= a->maxh);

    /* Fix parent relations. */
    parent = a->parent;
    pair->a = a;  a->parent = pair;
    pair->b = b;  b->parent = pair;
    pair->parent = parent;
    if (parent) {
	if (parent->a == a) {
	    parent->a = pair;
	} else {
	    assert(parent->b == a);
	    parent->b = pair;
	}
    }

    /* Set split info. */
    pair->size = size;
    pair->method = method;

    /* Set layout bounds. */
    if (is_horizontal_split(method)) {
	Uint16 d = abs(border.lft) + abs(border.rgt);
	b->minh = a->minh;
	b->maxh = a->maxh;
	b->minw = add_bounds(mind, d);
	b->maxw = add_bounds(maxd, d);
    } else /* horizontal layout */ {
	Uint16 d = abs(border.top) + abs(border.bot);
	b->minw = a->minw;
	b->maxw = a->maxw;
	b->minh = add_bounds(mind, d);
	b->maxh = add_bounds(maxd, d);
    }
    return b;
}

/* Auxiliary function to compute sizes *WA and *WB that use together
 * up to W pixels (in width or height), alloting PERCENTAGE of W to
 * *WB, but taking note of minimum and maximum bounds for the sizes of
 * A and B. */
static void
choose_sizes(
    Uint32 w,				/* available size. */
    Uint32 percentage,			/* percentage alloted to B. */
    Uint32 amin, Uint32 amax,		/* bounds for A. */
    Uint32 bmin, Uint32 bmax,		/* bounds for B. */
    Uint16 *wa,  Uint16 *wb)		/* result pointers. */
{
    if (w > amax + bmax) {
	*wa = amax;
	*wb = bmax;
    } else {
	Uint32 w1, w2;
	w2 = w * percentage / 100;
	w2 = min(w2, bmax);
	w2 = max(w2, bmin);

	w1 = w - w2;
	w1 = min(w1, amax);
	w1 = max(w1, amin);
	w2 = w - w1;
	*wa = w1;
	*wb = w2;
    }
}

/* Propose a layout for the layout PAIR into the W by H rectangle
 * whose top-left corner is at (X,Y), top-down.  The minimum and
 * maximum bounds for all pair-layouts must have been computed by
 * synthesize_bounds() before calling this function.  */
static void
propose_layout(Layout *pair, Sint16 x, Sint16 y, Uint16 w, Uint16 h)
{
    Layout *a = pair->a;
    Layout *b = pair->b;
    assert(w >= pair->minw);
    assert(h >= pair->minh);
    w = min(w, pair->maxw);
    h = min(h, pair->maxh);
    pair->p.r.x = x;
    pair->p.r.y = y;
    pair->p.r.w = w;
    pair->p.r.h = h;
    recompute_panel_view(&pair->p);
    if (!pair->a && !pair->b) {		/* A user's layout. */
	return;				/* all done! */
    } /* else a pair layout */

    /* Choose sizes based on the split parameters. */
    if (is_horizontal_split(pair->method)) {
	choose_sizes(w, pair->size,
		     a->minw, a->maxw, b->minw, b->maxw,
		     &a->p.r.w, &b->p.r.w);
	a->p.r.h = b->p.r.h = h;
	a->p.r.h = min(a->p.r.h, a->maxh); a->p.r.h = max(a->p.r.h, a->minh);
	b->p.r.h = min(b->p.r.h, b->maxh); b->p.r.h = max(b->p.r.h, b->minh);
    } else {
	choose_sizes(h, pair->size,
		     a->minh, a->maxh, b->minh, b->maxh,
		     &a->p.r.h, &b->p.r.h);
	a->p.r.w = b->p.r.w = w;
	a->p.r.w = min(a->p.r.w, a->maxw); a->p.r.w = max(a->p.r.w, a->minw);
	b->p.r.w = min(b->p.r.w, b->maxw); b->p.r.w = max(b->p.r.w, b->minw);
    }

    /* Make sure the layout algorithm respects all bounds. */
    assert(a->p.r.w >= a->minw);
    assert(a->p.r.w <= a->maxw);
    assert(b->p.r.w >= b->minw);
    assert(b->p.r.w <= b->maxw);
    assert(a->p.r.h >= a->minh);
    assert(a->p.r.h <= a->maxh);
    assert(b->p.r.h >= b->minh);
    assert(b->p.r.h <= b->maxh);
    assert(a->p.r.w <= w);
    assert(b->p.r.w <= w);
    assert(a->p.r.h <= h);
    assert(b->p.r.h <= h);
    if (is_horizontal_split(pair->method)) {
	assert(a->p.r.w + b->p.r.w <= w);
    } else {
	assert(a->p.r.h + b->p.r.h <= h);
    }

    /* Set positions based on the method of split and sizes. */
    switch(pair->method) {
    case BELOWS: /* b below a */
	a->p.r.x = x;
	a->p.r.y = y;
	b->p.r.x = x;
	b->p.r.y = y + a->p.r.h;
	break;
    case ABOVES: /* b above a */
	b->p.r.x = x;
	b->p.r.y = y;
	a->p.r.x = x;
	a->p.r.y = y + b->p.r.h;
	break;
    case LEFTS:  /* b to left of a */
	b->p.r.x = x;
	b->p.r.y = y;
	a->p.r.x = x + b->p.r.w;
	a->p.r.y = y;
	break;
    case RIGHTS: /* b to right of a */
	a->p.r.x = x;
	a->p.r.y = y;
	b->p.r.x = x + a->p.r.w;
	b->p.r.y = y;
	break;
    }
    recompute_panel_view(&a->p);
    recompute_panel_view(&b->p);

    /* Propose layouts for children. */
    propose_layout(pair->a, a->p.r.x, a->p.r.y, a->p.r.w, a->p.r.h);
    propose_layout(pair->b, b->p.r.x, b->p.r.y, b->p.r.w, b->p.r.h);
}

/* Redraw all borders of layouts below (and including) layout A.  Calls
 * the doclear() callback of layouts. */
static void
redraw_layout_borders(Layout *a)
{
    draw_border(&a->p.r, &a->p.b);
    if (a->a) { redraw_layout_borders(a->a); }
    if (a->b) { redraw_layout_borders(a->b); }
}

/* Clear all layouts below (and including) layout A.  Calls the
 * doclear() callback of layouts. */
static void
clear_layout(Layout *a)
{
    if (a->doclear) { a->doclear(&a->p, a->parms); }
    if (a->a) { clear_layout(a->a); }
    if (a->b) { clear_layout(a->b); }
}

/* Call the doremode() call backs of layouts.  These should be called
 * whenever the display mode changes. */
static void
remode_layout(Layout *a)
{
    if (a->a) { remode_layout(a->a); }
    if (a->b) { remode_layout(a->b); }
    if (a->doremode) { a->doremode(a->parms); }
}

/* Call the after_layout() call back of layouts.  The first one that
 * returns FALSE causes a quick return, so some call backs might not
 * be called. */
static int
callback_after_layouts(Layout *a)
{
    return (!a->a || callback_after_layouts(a->a))
	&& (!a->b || callback_after_layouts(a->b))
	&& (!a->after_layout || a->after_layout(&a->p, a->parms));
}

/* Call the before_layout() call backs of layouts below (and
 * including) A.  The first callback returning FALSE causes a quick
 * return, so some callbacks might not be called. */
static int
callback_before_layouts(Layout *a, Uint16 w, Uint16 h)
{
    return (!a->a || callback_before_layouts(a->a, w, h))
	&& (!a->b || callback_before_layouts(a->b, w, h))
	&& (!a->before_layout || a->before_layout(a, w, h));
}

/* Redraw a layout and all its children. */
static void
redraw_layout(Layout *a)
{
    if (a->doredraw) { (void)a->doredraw(&a->p, a->parms); }
    if (a->a) { redraw_layout(a->a); }
    if (a->b) { redraw_layout(a->b); }
}

/* Refresh the area on the display that the layout occupies. */
static void
refresh_layout(Layout *a)
{
    if (a->a) { refresh_layout(a->a); }
    if (a->b) { refresh_layout(a->b); }
    if (a->dorefresh) { (void)a->dorefresh(&a->p, a->parms); }
}

/* Attempt to recursively layout all the layouts below (and including)
 * the ROOT layout.  Returns TRUE on success, FALSE on failure. */
static int
layout(Layout *root, Sint16 x, Sint16 y, Uint16 w, Uint16 h)
{
    /* Let the user's layouts adjust themselves to the new width and height
     * of the display.
     */
    if (!callback_before_layouts(root, w, h)) {
	return 0;
    }

    /* Check that we have enough space. */
    synthesize_bounds(root);
    if (w < root->minw || h < root->minh)
    {
	return 0;
    }

    /* Wunderbar. Now compute a layout proposal into the layout structures
     * top-down. */
    propose_layout(root, x, y, w, h);

    /* Phew.  Now run the layout proposal past the user call-backs to see
     * if they agree. */
    return callback_after_layouts(root);
}

/* ------------------------------------------------------------------------
 * TextOutputs
 *
 * TextOutputs are Layouts that provide textual output in the current
 * font.  They have a cursor position, know how to wrap long lines
 * when necessary, and attempt to refresh only the parts of their
 * layout's panel that have been drawn on. */

/* Return a TextOutput to the statically allocated pool of free TextOutputs. */
static void
free_text_output(TextOutput *t)
{
    t->next = textoutput_free_list;
    textoutput_free_list = t;
}

/* Initialise the pool of free TextOutputs. */
static void
init_all_text_outputs(void)
{
    static int done_init = 0;
    if (!done_init) {
	int k;
	textoutput_free_list = NULL;
	for (k=MAXTEXTOUTPUTS-1; k>=0; --k) {
	    free_text_output(&all_textoutputs[k]);
	}
	done_init = 1;
    }
}

#if 0
static void
print_rect(const char *msg, const SDL_Rect *r)
{
    printf("%s %d,%d,%d,%d\n", msg, r->x, r->y, r->w, r->h);
}
#endif

static int
doclear_textoutput(Panel *p, void *parms)
{
    TextOutput *t = (TextOutput*)parms;
    /* clear_panel(p); */
    clear_rect(&t->drawn);
#if 0
    print_rect("cle", &t->drawn);
#endif
    t->refresh = t->drawn;		/* Set refresh rectangle. */
    t->x = t->y = 0;			/* Set point. */
    t->drawn.x = p->r.x;
    t->drawn.y = p->r.y;
    t->drawn.w = t->drawn.h = 0;
    return 1;
}

static int
after_layout_textoutput(Panel *p, void *parms)
{
    TextOutput *t = (TextOutput*)parms;
    t->refresh.w = t->refresh.h = 0;
    t->drawn = p->r;
    return 1;
}

static void
doclose_textoutput(void *parms)
{
    TextOutput *t = (TextOutput *)parms;
    free_text_output(t);
}

static int
dorefresh_textoutput(Panel *p, void *parms)
{
    TextOutput *t = (TextOutput *)parms;
    p=p;
    if (t->refresh.w && t->refresh.h) {
#if 0
	print_rect("ref", &t->refresh);
#endif
	refresh_rect(&t->refresh);
    }
    t->refresh = t->layout->p.r;
    t->refresh.w = t->refresh.h = 0;
    return 1;
}

static TextOutput *
alloc_text_output(Layout *a)
{
    TextOutput *t;
    init_all_text_outputs();
    t = textoutput_free_list;
    assert(t && "Ran out of text outputs");
    textoutput_free_list = t->next;
    memset((void*)t, 0, sizeof(TextOutput));
    t->layout = a;
    a->parms = t;
    a->after_layout = after_layout_textoutput;
    a->doclose = doclose_textoutput;
    a->doredraw = doclear_textoutput;
    a->doclear = doclear_textoutput;
    a->dorefresh = dorefresh_textoutput;
    t->x = t->y = 0;
    t->refresh.x = a->p.r.x;
    t->refresh.y = a->p.r.y;
    t->refresh.w = t->refresh.h = 0;
    t->drawn = t->refresh;
    return t;
}

static void
clear_to_eol(TextOutput *t)
{
    const SDL_Rect *r = panel_view(&t->layout->p);
    SDL_Rect v;
    v.x = r->x + t->x;
    v.y = r->y + t->y;
    v.h = CurrentFont.h;
    v.w = r->w;
    clip_rect(r, &v);
    Slock(TheSurf);
    SDL_FillRect(TheSurf, &v, Colours[BLACK]);
    Sunlock(TheSurf);
    refresh_rect(&v);
}

static void
cr(TextOutput *t)
{
    const SDL_Rect *r = panel_view(&t->layout->p);
    t->x = 0;
    /* Do we need to scroll up? */
    if (t->y+2*CurrentFont.h <= r->h) {
	/* No scrolling.  Move to the next line. */
	t->y += CurrentFont.h;
	clear_to_eol(t);
    } else {
	/* Scroll the panel up so that at least one more line fits and
	 * update the refresh rectangle to contain all of the panel.
	 * */
#if 0
	/* This version scrolls the minimum amount necessary to allow
	 * exactly one more line, but it turns out that that creates
	 * jerky and ugly displays.  i.e. it attempts to keep the
	 * bottom line aligned with the panel's bottom border.
	 * */
	int fh = CurrentFont.h;
	int ofs1 = r->h - (t->y+fh);
	int ofs2 = fh - ofs1;
	SDL_Rect src, dst;

	src = *r;			/* source rect */
	src.y += ofs2;
	src.h -= ofs2;
	clip_rect(&t->drawn, &src);

	dst = src;			/* dest rec. */
	dst.y -= ofs2;

	t->y = r->h - fh;		/* update point. */

	SDL_BlitSurface(TheSurf, &src, TheSurf, &dst);
	join_rect(&dst, &t->refresh);
	join_rect(&dst, &t->drawn);
#else
	/* This version scrolls up enough so that the top line is
	 * always aligned at the panel top border.  It leaves some unused
	 * space at the bottom, but that's probably ok. */
	int fh = CurrentFont.h;
	SDL_Rect src, dst;
	src = *r;
	src.y += fh;
	src.h -= fh;
	clip_rect(&t->drawn, &src);
	dst = src;
	dst.y -= fh;

	SDL_BlitSurface(TheSurf, &src, TheSurf, &dst);
	join_rect(&dst, &t->refresh);
	join_rect(&dst, &t->drawn);
#endif
	clear_to_eol(t);
    }
}

static void
emit(TextOutput *t, unsigned char c, int colix)
{
    const SDL_Rect *v = panel_view(&t->layout->p);
    if (c=='\n') { cr(t); return; }

    /* Do we need to wrap? */
    if (t->x+CurrentFont.w > v->w) {
	cr(t);
    }
    {
	SDL_Rect r;
	r.x = v->x + t->x; r.y = v->y + t->y;
	r.h = CurrentFont.h; r.w = CurrentFont.w;
	clip_rect(v, &r);
	outcharxy(c, r.x, r.y, colix, &r);
	t->x += CurrentFont.w;
	join_rect(&r, &t->refresh);
	join_rect(&r, &t->drawn);
    }
}

static void
backspace(TextOutput *t)
{
    t->x -= CurrentFont.w;
    if (t->x<0) {
	t->x = 0;
    }
}

static void
backchar(TextOutput *t)
{
    if (t->x >= CurrentFont.w) {
	t->x -= CurrentFont.w;
    } else {
	t->x = 0;
	if (t->y >= CurrentFont.h) {
	    int cols;
	    t->y -= CurrentFont.h;
	    panel_text_size(&t->layout->p, &cols, NULL);
	    t->x = max(0,(cols-1))*CurrentFont.w;
	}
    }
}

static void
delchar(TextOutput *t)
{
    backchar(t);
    emit(t, ' ', BLACK);
    backspace(t);
}

static void
type(TextOutput *t, const char *str, int colix)
{
    while (*str) {
	emit(t, *str, colix);
	if (*str==' ' && t->x <= CurrentFont.w) {
	    /* After wrapping text, don't emit a space as the very
	       first character of a line. */
	    backspace(t);
	}
	str++;
    }
}


static void
type_nowrap(TextOutput *t, const char *str, int colix)
{
    const SDL_Rect *v = panel_view(&t->layout->p);
    while (*str) {
	SDL_Rect r;
	r.x = v->x + t->x;
	r.y = v->y + t->y;
	r.w = CurrentFont.w;
	r.h = CurrentFont.h;
	clip_rect(v, &r);
	outcharxy(*str, r.x, r.y, colix, &r);
	t->x += CurrentFont.w;
	join_rect(&r, &t->refresh);
	join_rect(&r, &t->drawn);
	str++;
    }
}



/* ------------------------------------------------------------------------
 * TextInputs
 *
 * TextInputs supplement Layouts by providing text input facilities.
 * They don't do any event handling themselves, but track the internal
 * state of a text panel that an event handler can modify through a
 * narrowish interface.  An event handler should always activate() a
 * TextInput when it is starting to gather input, and deactivate() it
 * when input has been gathered.  The handler is also responsible for
 * refreshing the TextInput's Layout at convenient moments by calling
 * the refresh_layout() function on it's layout.
 *
 * A TextInput has two states:
 *
 *  active : 		When set, the TextInput is all geared up for receiving
 *			characters from the user (event handler).  When clear,
 *			the TextInput pretends it is an empty panel.  This
 *			variable may only be modified by calls to activate(),
 *			deactivate(), and finish_input().
 *
 * and a handful of minor boolean state variables:
 *
 *  has_focus :         When set and the TextInput is active, draw a cursor
 *			character to indicate that to the user.  This variable
 *			is controlled through the functions give_focus() and
 *			take_focus().
 *
 *  needs_prompting:	If set, the next time the TextInput is activated,
 *			it will redraw the prompt and any input it has
 *			gathered into its input buffer.  This flag should be
 *			set at any time that the TextInput's visible area is
 *			externally modified (by resize events or whatever.)
 **/

/* Return a TextInput to the statically allocated pool of free TextInputs. */
static void
free_text_input(TextInput *t)
{
    t->next = textinput_free_list;
    textinput_free_list = t;
}

/* Initialise the pool of free TextInputs. */
static void
init_all_text_inputs(void)
{
    static int done_init = 0;
    if (!done_init) {
	int k;
	textinput_free_list = NULL;
	for (k=MAXTEXTINPUTS-1; k>=0; --k) {
	    free_text_input(&all_textinputs[k]);
	}
	done_init = 1;
    }
}

/* Draw the cursor in a TextInput if it is active and has focus. */
static void
cursoron(TextInput *t)
{
    if (t->has_focus && t->active) {
	emit(t->out, '_', t->colix);
	backspace(t->out);
    }
}

/* Clear the cursor in a TextInput if it is active and has focus. */
static void
cursoroff(TextInput *t)
{
    if (t->has_focus && t->active) {
	emit(t->out, ' ', t->colix);
	backspace(t->out);
    }
}

/* Layout doredraw() callback redraws a text input if it is active. */
static int
doredraw_textinput(Panel *p, void *parms)
{
    TextInput *t = (TextInput*)parms;
    doclear_textoutput(p, t->out);
    if (t->active) {
	type(t->out, t->prompt, t->colix);
	type(t->out, t->buf, t->colix);
	cursoron(t);
    }
    return 1;
}

static int
after_layout_textinput(Panel *p, void *parms)
{
    TextInput *t = (TextInput*)parms;
    return after_layout_textoutput(p, t->out);
}

/* Layout dorefresh() callback refreshes the layout only when it has been
   drawn into. */
static int
dorefresh_textinput(Panel *p, void *parms)
{
    TextInput *t = (TextInput*)parms;
    dorefresh_textoutput(p, t->out);
    return 1;
}


/* Layout doclose() callback frees a related TextInput. */
static void
doclose_textinput(void *parms)
{
    TextInput *t = (TextInput*)parms;
    doclose_textoutput(t->out);
    free_text_input(t);
}

/* Allocate a new TextInput and relate it to the given Layout. */
static TextInput *
alloc_text_input(Layout *a)
{
    TextInput *t;
    init_all_text_inputs();
    t = textinput_free_list;
    assert(t && "Ran out of text input panels");
    textinput_free_list = t->next;
    memset((void*)t, 0, sizeof(TextInput));
    t->out = alloc_text_output(a);
    t->colix = LIGHTGREY;
    t->layout = a;
    a->parms = t;
    a->doclose = doclose_textinput;
    a->doredraw = doredraw_textinput;
    a->doclear = doredraw_textinput;
    a->dorefresh = dorefresh_textinput;
    a->after_layout = after_layout_textinput;
    t->active = 0;
    t->has_focus = 0;
    t->needs_prompting = 1;
    return t;
}

/* Activate a TextInput for receiving input. */
static void
activate(TextInput *t)
{
    if (t->needs_prompting) {
	type(t->out, t->prompt, t->colix);
	type(t->out, t->buf, t->colix);
    }
    t->active = 1;
    t->needs_prompting = 0;
    cursoron(t);
}

/* Deactivate a TextInput. */
static void
deactivate(TextInput *t)
{
    t->active = 0;
}

/* Is the TextInput receiving input? */
static int
is_active(TextInput *t)
{
    return t->active;
}

/* Give focus. */
static void
give_focus(TextInput *t)
{
    t->has_focus = 1;
    cursoron(t);
}

/* Remove focus. */
static void
take_focus(TextInput *t)
{
    cursoroff(t);
    t->has_focus = 0;
}

/* Set the prompt string of a TextInput. */
static void
set_prompt(TextInput *t, const char *prompt)
{
    xstrncpy(t->prompt, prompt, MAXALLCHAR);
}

/* Add a character to a TextInput. */
static void
add_char(TextInput *t, char c)
{
    if (!is_active(t)) { return; }
    if (t->len+1 < MAXALLCHAR) {
	t->buf[t->len++] = c;
	t->buf[t->len] = 0;
	if (c=='\n') { cursoroff(t); }	/* Remove lingering cursor. */
	emit(t->out, c, t->colix);
	if (c!='\n') { cursoron(t); }
    }
}

/* Erase the last character of a TextInput. */
static void
del_char(TextInput *t)
{
    if (!is_active(t)) { return; }
    if (t->len>0) {
	t->buf[--t->len] = 0;
	cursoroff(t);
	delchar(t->out);
	cursoron(t);
    }
}

/* Stop receiving input into the TextInput and fetch the input gathered
 * so far. */
static void
finish_input(TextInput *t, char *buf, size_t maxbuf)
{
    t->buf[t->len] = 0;
    xstrncpy(buf, t->buf, maxbuf);
    cursoroff(t);
    deactivate(t);
    t->needs_prompting = 1;
    t->buf[0] = 0;
    t->len = 0;
}

/* Erase the input characters on the screen up to the prompt.  NB: This
 * does not throw away gathered input or deactivate the TextInput. */
static void
hide_input(TextInput *t)
{
    size_t i;
    if (!is_active(t)) { return; }
    cursoroff(t);
    for (i=0; i<t->len; i++) {
	delchar(t->out);
    }
    cursoron(t);
}

static void
reset_input(TextInput *t, const char *s)
{
    if (!is_active(t)) { return; }
    hide_input(t);
    t->buf[0] = 0;
    t->len = 0;
    while (s && *s) { add_char(t, *s++); }
}

/* ------------------------------------------------------------------------
 * Core display layout
 */

/* Return the size of a cell in pixels. */
static Uint32
get_cell_size(const CellInfo *c)
{
    return 2*c->box_size + c->interbox_space + c->intercell_space;
}

/* Draw a cell-box at the given address in the given colour.  Bits TL
 * (top-left), TR (top-right), BL (bottom-left), BR (bottom-right) of BOXES
 * control which boxes are drawn in the cell.  Additionally it updates the
 * core colour buffer in the CoreArena. */
static void
display_box(int addr, Uint32 boxes, int colix)
{
    SDL_Rect dst;
    SDL_Surface *boxsurf;

    if (colix != 0) {
	/* Colour core colour buffer and figure out which boxes need
	 * colouring on the display. */
#if 1
	/* This version checks whether or not the box needs to be
	 * blitted.  Doesn't work when resizing because then the boxes
	 * always needs to be blitted so it's not used. */
#define check_box(mask, ofs)\
	if (boxes & (mask)) {\
	    if (CoreArena.core[4*addr+(ofs)] != colix) {\
		CoreArena.core[4*addr+(ofs)] = colix;\
	    } else {\
		boxes ^= (mask);\
	    }\
	}
#else
#define check_box(mask, ofs)\
	if (boxes & (mask)) { CoreArena.core[4*addr+(ofs)] = colix; }
#endif
	check_box(TL, 0);
	check_box(TR, 1);
	check_box(BL, 2);
	check_box(BR, 3);

	/* If TL=0x00000001, TR=0x00000100, BL=0x00010000, BR=0x01000000
	 * and CoreArena.core[] is an Uint32 array, then this might be faster
	 * since only DWORD's are store-forwaded(?):
	 *
	 * CoreArena.core[addr] = (CoreArena.core[addr]
	 *                         & ((boxes ^ (TL|TR|BL|BR)) * 0xFF)
	 *                        )  |  (boxes * colix);
	 */
	if (boxes) {
	    /* Set destination rectangle. */
	    dst.x = xkoord(addr);
	    dst.y = ykoord(addr);
	    /* dst.w = dst.h = CellSize; */  /* ignored by SDL_BlitSurface().*/

	    /* Get source surface. */
	    boxsurf = BlitBoxes[colix][boxes];

	    /* Blit it!  Note: requires colour keying to select area to blit.*/
	    SDL_BlitSurface(boxsurf, NULL, TheSurf, &dst);
	}
    }
}

/* Decrease the parameter cell size parameters a bit.  This is used to find a
 * suitable cell size that fits the screen.  Returns a flag that is zero when
 * the cell size can't be decreased any more, and if it can. */
static int
decr_cell_size(CellInfo *c)
{
    int success=1;
    if (c->interbox_space > 1) {
	/* Try to keep boxes further apart than one pixel. */
	--c->interbox_space;
    } else if (c->intercell_space > 1) {
	/* Try to keep cells further apart than boxes. */
	--c->intercell_space;
    } else if (c->interbox_space > 0) {
	/* Give in and make boxes touch each other. */
	c->interbox_space = 0;
    } else if (c->intercell_space >= 1) {
	/* Give in and make cells and boxes the same distance apart. */
	c->intercell_space = 0;
    } else if (c->box_size>1) {
	/* Try to keep boxes larger than one pixel */
	--c->box_size;
	if (c->intercell_space==0) {
	    c->intercell_space = 1;
	}
    } else {
	assert(c->box_size == 1);
	assert(c->interbox_space == 0);
	assert(c->intercell_space == 0);
	success = 0;
    }
    return success;
}

/* Compute the maximum and minimum height of the core display panel
 * based on the available width.  */
static int
before_layout_arena(Layout *layout, Uint16 w, Uint16 h)
{
    Panel *p = &layout->p;
    Arena *a = (Arena*)(layout->parms);
    Uint32 real_w = max(w, layout->minw) - abs(p->b.lft) - abs(p->b.rgt);
    Uint32 cellsperline;

    h = h;				/* unused. */

    cellsperline = real_w/2;		/* 2 = minimum cell size. */
    layout->minh = 2*(a->coresize + cellsperline-1)/cellsperline;
    layout->minh += abs(p->b.top) + abs(p->b.bot);

    layout->maxh = max(layout->maxh, layout->minh);
    return 1;
}

/* After the core panel has been laid out, find a cell size that will
 * fit into the alotted space.  Also set up related globals.  Provided
 * the panel is large enough, this should always succeed.  */
static int
after_layout_arena(Panel *p, void *parms)
{
    const SDL_Rect *r = panel_view(p);
    Arena *a = (Arena*)parms;
    int failed = 1;
    Uint32 rows, cellw, cellsperline;

    /* Find desired cell size. */
    a->cell = a->desired_cell;
    do {
	cellw = get_cell_size(&a->cell);
	cellsperline = r->w / cellw;
	rows = (a->coresize + cellsperline-1)/cellsperline;
#if 0
	fprintf(stderr, "{ %d, %d, %d } = %d/%d\n", a->cell.box_size,
		a->cell.interbox_space,	a->cell.intercell_space,
		rows*cellw, r->h);
#endif
	if (rows * cellw <= r->h) {
	    failed = 0;
	}
    } while(failed && decr_cell_size(&a->cell));

#if 0
	fprintf(stderr, "{ %d, %d, %d } = %d/%d\n", a->cell.box_size,
		a->cell.interbox_space,	a->cell.intercell_space,
		rows*cellw, r->h);
#endif
    /* Set up related globals. */
    ArenaX = r->x;
    ArenaY = r->y;
    ArenaX += (r->w - cellw*cellsperline + a->cell.intercell_space)/2;
    ArenaY += (r->h - cellw*rows + a->cell.intercell_space)/2;
    CellSize = get_cell_size(&a->cell);
    CellsPerLine = r->w/CellSize;

    /* If there's wasted horizontal space in the arena, adjust the
     * maximum height and fail this layout.  The layout algorithm
     * should try again and get it right. */
    if (r->h > rows*cellw + a->cell.intercell_space
	     + abs(p->b.top) + abs(p->b.bot))
    {
	a->layout->maxh = a->cell.intercell_space + rows*cellw;
	a->layout->maxh += abs(p->b.top) + abs(p->b.bot);
	a->layout->minh = min(a->layout->minh, a->layout->maxh);
	failed = 1;
    }

    return !failed;
}

/* Layout doredraw() callback redraws the core panel from the core colour
 * buffer. */
static int
doredraw_arena(Panel *p, void *parms)
{
    Arena *a = (Arena*)parms;
    Uint32 addr;
    clear_panel(p);
    for (addr=0; addr<a->coresize; addr++) {
	int i;
	Uint32 mask = 1;
	for (i=0; i<4; i++) {
	    unsigned char col = a->core[4*addr+i];
	    a->core[4*addr + i] = 0;
	    display_box(addr, mask, col);
	    mask <<= 1;
	}
    }
    return 1;
}

/* Layout doclear() callback clears the arena and core colour buffer. */
static int
doclear_arena(Panel *p, void *parms)
{
    Arena *a = (Arena*)parms;
    a = a;
    clear_panel(p);
    memset((void*)a->core, 0, sizeof(unsigned char)*4*a->coresize);
    return 1;
}

/* Free the blit boxes array. */
static void
free_blitboxes(Arena *a)
{
    int colix, boxes;
    if (a->blitboxes==NULL) { return; }
    for (colix=0; colix<NCOLOURS; colix++) {
	if (a->blitboxes[colix]) {
	    for (boxes = 0; boxes < 16; boxes++) {
		SDL_Surface *s = a->blitboxes[colix][boxes];
		if (s) {
		    SDL_FreeSurface(s);
		    a->blitboxes[colix][boxes] = NULL;
		}
	    }
	    FREE(a->blitboxes[colix]);
	}
    }
    FREE(a->blitboxes);
    a->blitboxes = NULL;
}

/* Fill a square box of a surface with a colour. */
static void
fill_box(SDL_Surface *s, Sint16 x, Sint16 y, Uint16 w, Uint16 h, Uint32 col)
{
    SDL_Rect r;
    r.x = x; r.y = y; r.w = w; r.h = h;
    if (-1==SDL_FillRect(s, &r, col)) {
	exit_panic(errSpriteFill, SDL_GetError());
    }
}

/* Predraw a blitbox of a given colour and pattern. */
static void
make_blitbox(Arena *a, int colix, Uint32 boxes)
{
    SDL_Surface *sprite, *temp;
    Uint32 key, col;
    int cellsize = get_cell_size(&a->cell);
    int bs = a->cell.box_size;
    int ibsp = a->cell.interbox_space;

    sprite = SDL_CreateRGBSurface(SDL_ANYFORMAT, cellsize, cellsize,
				  TheSurf->format->BitsPerPixel,
				  TheSurf->format->Rmask,
				  TheSurf->format->Gmask,
				  TheSurf->format->Bmask,
				  0);	/* no alpha, we want colorkey blits */
    if (sprite == NULL) {
	exit_panic(errSpriteSurf, SDL_GetError());
    }

    /* Set colorkey (=black) for the sprite. */
    key = SDL_MapRGB(sprite->format, 0, 0, 0);
    if (-1==SDL_SetColorKey(sprite, SDL_SRCCOLORKEY | SDL_RLEACCEL, key)) {
	exit_panic(errSpriteColKey, SDL_GetError());
    }


    /* Draw the sprite. */
    col = SDL_MapRGB(sprite->format,
		     PaletteRGB[colix].r,
		     PaletteRGB[colix].g,
		     PaletteRGB[colix].b);
    fill_box(sprite, 0, 0, cellsize, cellsize, key);
    if (boxes & TL) {
	fill_box(sprite,       0,       0, bs, bs, col);
    }
    if (boxes & BL) {
	fill_box(sprite,       0, bs+ibsp, bs, bs, col);
    }
    if (boxes & TR) {
	fill_box(sprite, bs+ibsp,       0, bs, bs, col);
    }
    if (boxes & BR) {
	fill_box(sprite, bs+ibsp, bs+ibsp, bs, bs, col);
    }

    /* Convert sprite to video format. */
    temp = SDL_DisplayFormat(sprite);
    SDL_FreeSurface(sprite);
    if (temp == NULL) {
       exit_panic(errSpriteConv, SDL_GetError());
    }
    sprite = temp;

    /* Save it for later! */
    a->blitboxes[colix][boxes] = sprite;
}

/* Predraw the blitboxes for a given colour. */
static void
make_blitboxes_for_colix(Arena *a, int colix)
{
    int i;

    /* Allocate the blitboxes[] array? */
    if (a->blitboxes == NULL) {
	a->blitboxes = (SDL_Surface***)MALLOC(sizeof(SDL_Surface**)*NCOLOURS);
	if (a->blitboxes==NULL) {
	    exit_panic(outOfMemory, NULL);
	}
	for (i=0; i<NCOLOURS; i++) {
	    a->blitboxes[i] = NULL;
	}
    }

    /* Make the boxes for this colour index? */
    if (a->blitboxes[colix] == NULL) {
	a->blitboxes[colix] = (SDL_Surface**)MALLOC(sizeof(SDL_Surface*)*16);
	if (a->blitboxes[colix] == NULL) {
	    exit_panic(outOfMemory, NULL);
	}
	for (i=0; i<16; i++) {
	    make_blitbox(a, colix, i);
	}
    }
}


/* Close the core panel. */
static void
doclose_arena(void *parms)
{
    Arena *a = (Arena*)parms;
    free_blitboxes(a);
    if (a->core) {
	FREE((void*)a->core);
	a->core = NULL;
    }
}

/* Layout doremode() callback initiates the predraw of BlitBoxes used for fast
 * blitting of core cells. */
static void
doremode_arena(void *parms)
{
    Arena *a = (Arena*)parms;
    int warix;
    free_blitboxes(a);

    for (warix=0; warix<warriors; warix++) {
	make_blitboxes_for_colix(a, WarColourIx[warix]);
	make_blitboxes_for_colix(a, DieColourIx[warix]);
    }
    BlitBoxes = a->blitboxes;
}

static CellInfo
choose_cell(int mode)
{
    mode = min(9, mode);
    mode = max(0, mode);
    return DesiredCells[mode];
}


/* Initialise the Layout LAYOUT to be the CoreArena for a core of size
 * CORESIZE. */
static void
init_arena(Layout *layout, size_t coresize, int displayMode)
{
    Arena *a = &CoreArena;

    a->coresize = coresize;
    a->core = (unsigned char*)MALLOC(sizeof(char)*4*coresize);
    if (!a->core) { exit_panic(outOfMemory, NULL); }
    memset((void*)a->core, 0, sizeof(unsigned char)*4*coresize);

    a->cell = choose_cell(displayMode);

    a->layout = layout;
    layout->parms = a;
    layout->before_layout = before_layout_arena;
    layout->after_layout = after_layout_arena;
    layout->doredraw = doredraw_arena;
    layout->doclear = doclear_arena;
    layout->doclose = doclose_arena;
    layout->doremode = doremode_arena;
}

/* ------------------------------------------------------------------------
 * Status line.
 */

/* Layout doredraw() callback that draws the status line.  Depends on
 * global inCdb. */
static int
doredraw_status(Panel *p, void *parms)
{
    TextOutput *t = (TextOutput *)parms;
    const SDL_Rect *r = panel_view(p);
    int white = WHITE;
    int red = RED;
    int yellow = YELLOW;
    int i;

    clear_panel(p);
    doclear_textoutput(p, parms);

    /* Speed level meter. */
    type_nowrap(t, "<", white);
    for (i=0; i<SPEEDLEVELS - displaySpeed; i++) {
	if (t->x+CurrentFont.w+1 <= r->w) {
	    outblankxy(r->x+t->x, r->y, red, r);
	    t->x += CurrentFont.w+1;
	}
    }
    for (i=0; i<displaySpeed; i++) {
	if (t->x+CurrentFont.w <= r->w) {
	    outblankxy(r->x+t->x+1, r->y, yellow, NULL);
	    t->x += CurrentFont.w+1;
	}
    }
    type_nowrap(t, ">", white);

    /* Detail level. */
    type_nowrap(t, "    ", white);
    for (i=0; i<5; i++) {
	Uint32 col = i!=displayLevel ? white : red;
	char s[10];
	sprintf(s,"%d ", i);
	type_nowrap(t, s, col);
    }

    /* Status */
    type_nowrap(t, "    ", white);
    if (inCdb) {
	type_nowrap(t, "Debug", red);
    } else {
	type_nowrap(t, "Debug space Quit", white);
    }
    return 1;
}

/* ------------------------------------------------------------------------
 * Warrior names and meters.
 */

/* Layout doredraw() callback types the names of the warriors into the warrior
 * names layout. */
static int
doredraw_names(Panel *p, void *parms)
{
    TextOutput *t = (TextOutput*)parms;
    const SDL_Rect *r = panel_view(p);
    int i;

    doclear_textoutput(p, parms);
    for (i=0; i<warriors; i++) {
	t->y = 0;			/* space names evenly in panel. */
	t->x = i*r->w/warriors;
	type_nowrap(t, warrior[i].name, WarColourIx[i]);
    }
    return 1;
}

/* Redraw a meter that is at offset YOFS in the rectangle R, has a count of
 * COUNT, a maximum count of MAXCOUNT, and a pixels per count ratio of
 * RATIO. */
static void
redraw_meter(const SDL_Rect *r, int yofs, int count, int maxcount, int ratio,
	     Uint32 col)
{
    SDL_Rect q;

    Slock(TheSurf);
    putpixel(TheSurf, r->x + 1 + maxcount/ratio, r->y + yofs, col);
    Sunlock(TheSurf);

    q.x = r->x;  q.y = r->y + yofs;
    q.h = 1; q.w = 1 + count/ratio;
    SDL_FillRect(TheSurf, &q, col);
}

/* Layout doredraw() callback redraws the cycle and process meters. */
static int
doredraw_meters(Panel *p, void *parms)
{
    const SDL_Rect *r = panel_view(p);
    int i;

    parms = parms;
    clear_panel(p);
    for (i=0; i<warriors; i++) {
	Uint32 col = Colours[WarColourIx[i]];
	redraw_meter(r, splY[i]-r->y, warrior[i].tasks, taskNum,
		     processRatio, col);
    }

    redraw_meter(r, cycleY-r->y, cycle, warriors*cycles,
		 cycleRatio, Colours[WHITE]);
    return 1;
}

/* Layout after_layout() callback sets Y-coordinates and ratios of the various
 * meters. */
static int
after_layout_meters(Panel *p, void *parms)
{
    const SDL_Rect *r = panel_view(p);
    int i;
    parms = parms;
    for (i=0; i<warriors; i++) {
	splY[i] = r->y + 2*i;
    }
    processRatio = (taskNum + r->w-2)/(r->w-1);
    processRatio = max(1, processRatio);

    MetersX = r->x;
    cycleY = r->y + 2*warriors+2;
    cycleRatio = (2*warriors*cycles + r->w-2)/(r->w-1);
    cycleRatio = max(1, cycleRatio);
    return 1;
}

/* ------------------------------------------------------------------------
 * Resize/Redraw
 */

/* Redraw everything from scratch and update the display. */
static void
redraw(void)
{
    Layout *root = get_root_layout(CoreLayout);
    redraw_layout(root);
    SDL_Flip(TheSurf);
}

/* Layout everything into a display of size W by H pixels, enlarging the
 * display if necessary.  This is the only function that calls set_VMode() to
 * open/resize the display, so it also initiates any one-time display-format
 * specific processing. */
static void
relayout(Uint16 w, Uint16 h)
{
#define BW 3
#define BH 1
    Sint16 oldw = TheSurf ? TheVMode.w : -1;
    Sint16 oldh = TheSurf ? TheVMode.h : -1;
    Layout *root = get_root_layout(CoreLayout);
    CoreLayout->maxh = UNBOUNDED;
    CoreArena.desired_cell = choose_cell(displayMode);

    w = max(2*BW, w);
    h = max(2*BH, h);

    /* Attempt to compute a suitable layout. */
    if (!layout(root, BW, BH, w-2*BW, h-2*BH)) {
	/* Failed with the desired width/height.  Try again by adjusting for
	 * the minimum bounds. */
	if (w <= root->minw+2*BW) { w = root->minw + 2*BW; }
	if (h <= root->minh+2*BH) { h = root->minh + 2*BH; }

	if (!layout(root, BW, BH, w-2*BW, h-2*BH)) {
	    if (w <= root->minw+2*BW) { w = root->minw + 2*BW; }
	    if (h <= root->minh+2*BH) { h = root->minh + 2*BH; }
	    if (!layout(root, BW, BH, w-2*BW, h-2*BH)) {
		/* Still no layout.  Bail. */
		exit_panic("Couldn't compute a suitable layout", "BUG");
	    }
	}
    }

    if (!TheSurf || oldw != w || oldh != h) {
	if (TheSurf) {
	    SDL_FreeSurface(TheSurf);
	    TheSurf = NULL;
	}
	TheVMode.w = w;
	TheVMode.h = h;
	TheSurf = set_VMode(&TheVMode);
	if (!TheSurf) {
	    exit_panic(errDisplayOpen, SDL_GetError());
	}
    }
#if 0
    printf("relayout ok\n");
#endif
    remode_layout(root);
    redraw();
}

/* Initialise the global layout structures.  The display does not need to be
 * open. */
static void
init_layout(void)
{
    int i;
    Layout *names = NULL;

    /* Open arena layout. */
    CoreLayout = new_layout(200, UNBOUNDED,
			    0, UNBOUNDED, border(2,2,2,2,WHITE));
    init_arena(CoreLayout, coreSize, displayMode);

    /* Open meters panel. */
    MetersLayout =
	split_layout(CoreLayout, ABOVES, 0, 2*warriors+3, 2*warriors+3,
		     border(-2,-2,-2,-2,WHITE));
    MetersLayout->minw = 2;
    MetersLayout->doredraw = doredraw_meters;
    MetersLayout->doclear = doredraw_meters;
    MetersLayout->after_layout = after_layout_meters;

    /* Open names panel. */
    if (warriors <= 2) {
	names = split_layout(MetersLayout, ABOVES, 0,
			     CurrentFont.h, CurrentFont.h,
			     noborder);
	names->parms = alloc_text_output(names);
	names->doredraw = names->doclear = doredraw_names;
    }

    /* Open the status line panel. */
    StatusLayout = split_layout(CoreLayout, BELOWS, 0,
				CurrentFont.h, CurrentFont.h, noborder);
    StatusLayout->parms = alloc_text_output(StatusLayout);
    StatusLayout->doredraw = doredraw_status;
    StatusLayout->doclear = doredraw_status;

    /* Initialise input panels.  We open an input panel that takes 0% of
     * the space of the core.  The layout algorithm will see to it that
     * whatever space is left over from the core will be given to the text
     * panels.
     */
    for (i=0; i<MAXTEXTINPUTS; i++) { TextPanels[i] = NULL; }
    TextPanels[0] =
	alloc_text_input(
	    split_layout(StatusLayout, BELOWS, 0, 2*CurrentFont.h, UNBOUNDED,
			 noborder));
    set_prompt(TextPanels[0], CDB_PROMPT);
    give_focus(TextPanels[0]);
    NTextPanels = 1;
    curPanel = 1;

    /* Set warrior colours. */
    if (warriors <= 2) {
	WarColourIx[0] = GREEN;
	DieColourIx[0] = WarColourIx[0] % (NCOLOURS-1) + 1;
	WarColourIx[1] = LIGHTRED;
	DieColourIx[1] = WarColourIx[1] % (NCOLOURS-1) + 1;
    } else {
	int i;
	for (i=0; i<MAXWARRIOR; i++) {
	    WarColourIx[i] = DieColourIx[i] = ((9-1+i) % (NCOLOURS-1)) + 1;
	}
    }
}

/* ------------------------------------------------------------------------
 * Layout interface
 */

/* Clear the core arena. */
void
sdlgr_clear_arena(void)
{
    clear_layout(CoreLayout);
    refresh_layout(CoreLayout);
}

/* Clear the arena and redraw the meters. */
void
sdlgr_display_clear(void)
{

    clear_layout(CoreLayout);
    redraw_layout(MetersLayout);
    refresh_layout(CoreLayout);
    refresh_layout(MetersLayout);
}

/* Relayout everything. */
void
sdlgr_relayout(void)
{
    relayout(TheVMode.w, TheVMode.h);
}

/* Refresh things on the screen. */
void
sdlgr_refresh(int what)
{
    switch (what) {
    case -1:
	refresh_layout(get_root_layout(CoreLayout));
    case 0:
	refresh_layout(CoreLayout);
	break;
    default: {
	    int i;
	    for (i=0; i<NTextPanels; i++) {
		refresh_layout(TextPanels[i]->layout);
	    }
	    break;
	}
    }
}

/* Redraw the status line. */
void
sdlgr_write_menu(void)
{
    redraw_layout(StatusLayout);
    refresh_layout(StatusLayout);
}

/* ------------------------------------------------------------------------
 * Text Panels
 */

/* Create new text panels by splitting an old one. */
static int
split_text_panel(int panel, int size, Uint16 method)
{
    assert(1 <= panel && panel <= NTextPanels);
    assert(TextPanels[panel-1]);
    size = min(100, size);
    size = max(0, size);

    if (NTextPanels < MAXTEXTINPUTS) {
	Layout *a = TextPanels[panel-1]->layout;
	Layout *b;
	TextInput *t;
	SDL_Rect r = a->p.r;
	int newpanel;
	b = split_layout(a,
			 method, size,
			 0, UNBOUNDED,
			 noborder);
	b->minh = CurrentFont.h;
	b->minw = CurrentFont.w;

	TextPanels[NTextPanels] = t = alloc_text_input(b);
	set_prompt(t, CDB_PROMPT);
	newpanel = ++NTextPanels;

	/* Attempt to re-layout only the pair layout that contains the
	 * new text panel. If it fails, then do a full re-layout.
	 * This avoids having to redraw the arena on an update that
	 * doesn't affect the core panel.  */
	if (layout(a->parent, r.x, r.y, r.w, r.h)) {
	    redraw_layout(b);
	    refresh_layout(b);
	} else {
	    relayout(TheVMode.w, TheVMode.h);
	}
	return newpanel;
    }
    return -1;
}

/* Close a text panel. */
static void
close_text_panel(int panel)
{
    int i;
    SDL_Rect r;
    int need_redraw = 0;
    Layout *a, *b;

    if (panel < 1 || panel > NTextPanels) { return; }
    if (TextPanels[panel-1] == NULL) { return; }
    if (NTextPanels <= 1) { return; }

    a = TextPanels[panel-1]->layout;
    b = get_sibling_layout(a);
    r = a->parent->p.r;
    need_redraw = r.x != b->p.r.x || r.y != b->p.r.y;
    clear_layout(a);
    refresh_layout(a);
    close_layout(a);
    TextPanels[panel-1] = NULL;

    /* Shift the remaining panels down. */
    for (i=panel; i<NTextPanels; i++) {
	TextPanels[i-1] = TextPanels[i];
    }
    --NTextPanels;
    TextPanels[NTextPanels] = NULL;

    if (layout(b, r.x, r.y, r.w, r.h)) {
	if (need_redraw) {
	    TextInput *t = (TextInput*)(b->parms);
	    t->needs_prompting = 1;
	    clear_layout(b);
	    refresh_layout(b);
	}
    } else {
	relayout(TheVMode.w, TheVMode.h);
    }
}

/* Update the cdb text panels.  If NEXTPANEL == 0 then the current panel is
 * closed.  Otherwise, set focus to the panel NEXTPANEL (creating a new one if
 * NEWPANEL doesn't already exist).  The global curPanel is updated to reflect
 * the new panel.  NOTE: curPanel may be != NEXTPANEL if a new panel had to be
 * created, in which case curPanel is the id of the new panel. */
void
sdlgr_update(int nextpanel)
{
    /* Initialise? */
    if (curPanel <= 0) {
	curPanel = 1;
	assert(TextPanels[0]!=NULL);
    }

    /* Is this a close request? */
    if (nextpanel == 0) {
	close_text_panel(curPanel);
	nextpanel = curPanel <= NTextPanels ? curPanel : NTextPanels;
    } else {
	take_focus(TextPanels[curPanel-1]);
	/* Do we need to create a new panel? */
	if (TextPanels[nextpanel-1] == NULL) {
	    nextpanel = split_text_panel(NTextPanels, 50, RIGHTS);
	    if (nextpanel<=0) {		/* split failed? */
		return;			/* yup, give up. */
	    }
	}
    }
    curPanel = nextpanel;
    give_focus(TextPanels[curPanel-1]);
}

/* Clear the current panel. */
void
sdlgr_clear(void)
{
    Layout *a = TextPanels[curPanel-1]->layout;
    clear_layout(a);
}

/* Type a string to the current panel. */
void
sdlgr_puts(const char *s)
{
    TextOutput *t = TextPanels[curPanel-1]->out;
    int colix = TextPanels[curPanel-1]->colix;
    if (printAttr) {
	colix = WarColourIx[printAttr-1];
    }
    type(t, s, colix);
}

/* Get the number of rows in the current panel. */
int
sdlgr_text_lines(void)
{
    int rows;
    Panel *p = &TextPanels[curPanel-1]->layout->p;
    panel_text_size(p, NULL, &rows);
    return rows;
}


/* ------------------------------------------------------------------------
 * History ring
 *
 * This section provides command line history facilities for the event
 * handlers to use.
 **/

static Cmd *
get_cmd_ring()
{
    static int done_init = 0;
    if (!done_init) {
	CmdRing.cmd = xstrdup("");
	CmdRing.prev = CmdRing.next = &CmdRing;
	done_init = 1;
    }
    return &CmdRing;
}

static void
add_cmd(const char *cmd)
{
    Cmd *ring = get_cmd_ring();
    Cmd *c = MALLOC(sizeof(Cmd));
    if (c == NULL) {
	exit_panic(outOfMemory, NULL);
    }
    c->cmd = xstrdup(cmd);
    { char *s = c->cmd; while (s && *s) { if (*s=='\n') { *s='\0'; } s++; } }
    c->next = ring->next;
    c->prev = ring;
    ring->next->prev = c;
    ring->next = c;
}


/* ------------------------------------------------------------------------
 * Keyboard and mouse input; event handlers.
 */
#define any_set(bits,mask)  (((bits)&(mask))!=0)

/* Handle VIDEORESIZE, VIDEOEXPOSE, and QUIT events. */
static void
default_handler(SDL_Event *e)
{
#ifdef WIN32
    /* Under windows we ignore the very first VIDEOEXPOSE event. */
    static int nvideoexpose_events = 0;
#endif
    switch (e->type) {
    case SDL_VIDEORESIZE:
	relayout(e->resize.w, e->resize.h);
	SDL_Flip(TheSurf);
	break;
    case SDL_VIDEOEXPOSE:
#ifdef WIN32
	if (++nvideoexpose_events > 1) {
	    redraw();
	}
#endif
	break;
    case SDL_QUIT:
	sdlgr_display_close(WAIT);
	Exit(USERABORT);
    }
}

/* Handle special key strokes: CTRL-C, CTRL-BREAK, ALT-RETURN.
 * Returns 1 if the key is handled, and 0 otherwise.  */
static int
special_keyhandler(const SDL_keysym *s)
{
    /* Process CTRL-C, CTRL-BREAK */
    if (any_set(s->mod, KMOD_CTRL)
	&& (s->sym == SDLK_c
	    || s->sym==SDLK_BREAK))
    {
	SDL_Event e;
	e.type = SDL_QUIT;
	default_handler(&e);		/* shouldn't return. */
    }

    /* Toggle fullscreen mode on ALT-RETURN */
    if ((any_set(s->mod, KMOD_ALT)
	 || any_set(s->mod, KMOD_MODE))
	&& (s->sym == SDLK_RETURN))
    {
	SDL_WM_ToggleFullScreen(TheSurf);
	return 1;
    }
    return 0;
}


/** Deal with keypad and other conversion weirdness. */
static void
normalise_keysym(SDL_keysym *s)
{
    int num = any_set(s->mod, KMOD_NUM);

    /* Windows (or perhaps SDL) doesn't handle keystrokes with AltGr too well.
     * Problems include: spurious modifier bits, iffy unicode conversion for
     * valid international keys, varying keysyms on different versions of windows,
     * and so on and so forth.  Ignoring all keys modified by AltGr doesn't work
     * well because some characters (e.g. @ $) can only be typed using AltGr
     * on scandinavian keyboards.
     */
    if (any_set(s->mod, KMOD_RALT)) {
	s->mod = KMOD_RALT;
    }

    /* Keypad */
    switch (s->sym) {
    case SDLK_KP0:
	if (num) { s->sym = s->unicode = '0'; } else { s->sym = SDLK_INSERT; }
	break;
    case SDLK_KP1:
	if (num) { s->sym = s->unicode = '1'; } else { s->sym = SDLK_END; }
	break;
    case SDLK_KP2:
	if (num) { s->sym = s->unicode = '2'; } else { s->sym = SDLK_DOWN; }
	break;
    case SDLK_KP3:
	if (num) { s->sym = s->unicode = '3'; } else { s->sym = SDLK_PAGEDOWN;}
	break;
    case SDLK_KP4:
	if (num) { s->sym = s->unicode = '4'; } else { s->sym = SDLK_LEFT; }
	break;
    case SDLK_KP5:
	if (num) { s->sym = s->unicode = '5'; }
	break;
    case SDLK_KP6:
	if (num) { s->sym = s->unicode = '6'; } else { s->sym = SDLK_RIGHT; }
	break;
    case SDLK_KP7:
	if (num) { s->sym = s->unicode = '7'; } else { s->sym = SDLK_HOME; }
	break;
    case SDLK_KP8:
	if (num) { s->sym = s->unicode = '8'; } else { s->sym = SDLK_UP; }
	break;
    case SDLK_KP9:
	if (num) { s->sym = s->unicode = '9'; } else { s->sym = SDLK_PAGEUP; }
	break;
    }

}

/* Convert a key from a SDL_keysym structure to ASCII.  Returns 0 if
* the key isn't an ASCII character, and the character otherwise. */
static char
conv_key(const SDL_keysym *s)
{
    if (s->unicode >= 128) { return 0; } /* non-ASCII */
    if (any_set(s->mod, KMOD_CTRL | KMOD_LALT | KMOD_META)) {
	return 0;		/* special, modified */
    }
    switch (s->sym) {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
	return '\n';
    case SDLK_TAB:
	return '\t';
    case SDLK_DELETE:
    case SDLK_BACKSPACE:
	return '\b';
    case SDLK_ESCAPE:
	return 27;			/* ASCII for escape. */
    default:
	return s->unicode;
    }
    return 0;
}

static void
print_keysym(const SDL_keysym *s)
{
    fprintf(stderr,"sym: %d, unicode: %d '%c', mod: %x\n",
	    s->sym, s->unicode,
	    isgraph(s->unicode) ? s->unicode : '.',
	    s->mod);
}


/* Do special processing for macro keys: if the key is a modified
 * key or non-ASCII key (function key, arrow keys, etc.) return the
 * name of the key in the form " m <modifiers><keyname>\n" in the
 * buffer BUF.  Returns 1 if an expansion was made, and 0 if not. */
static int
macro_key(const SDL_keysym *s, char *buf, int maxbuf)
{
    char name[MAXALLCHAR];
    int ok = 0;
    int modified = 0;
    static const size_t n = MAXALLCHAR;
    if (s->sym == SDLK_UNKNOWN) { return 0; }

    xstrncpy(name, " m ", n);
    if (any_set(s->mod, KMOD_CTRL)) {
	xstrncat(name, "ctrl-", n); modified = 1;
    }
    if (any_set(s->mod, KMOD_LALT)) {
	xstrncat(name, "alt-", n); modified = 1;
    }
    if (any_set(s->mod, KMOD_META)) {
	xstrncat(name, "meta-", n); modified = 1;
    }

    switch (s->sym) {
    case SDLK_UP:
	xstrncat(name, "up", n); ok=1;
	break;
    case SDLK_DOWN:
	xstrncat(name, "down", n); ok=1;
	break;
    case SDLK_RIGHT:
	xstrncat(name, "right", n); ok=1;
	break;
    case SDLK_LEFT:
	xstrncat(name, "left", n); ok=1;
	break;
    case SDLK_INSERT:
	xstrncat(name, "ins", n); ok=1;
	break;
    case SDLK_HOME:
	xstrncat(name, "home", n); ok=1;
	break;
    case SDLK_END:
	xstrncat(name, "end", n); ok=1;
	break;
    case SDLK_PAGEUP:
	xstrncat(name, "pgup", n); ok=1;
	break;
    case SDLK_PAGEDOWN:
	xstrncat(name, "pgdn", n); ok=1;
	break;
    case SDLK_F1:    case SDLK_F2:    case SDLK_F3:    case SDLK_F4:
    case SDLK_F5:    case SDLK_F6:    case SDLK_F7:    case SDLK_F8:
    case SDLK_F9:    case SDLK_F10:    case SDLK_F11:    case SDLK_F12:
    case SDLK_F13:    case SDLK_F14:    case SDLK_F15:
	{
	    char keyname[5];
	    sprintf(keyname, "f%d", 1+s->sym-SDLK_F1);
	    xstrncat(name, keyname, n); ok=1;
	    break;
	}
    case SDLK_HELP:
	xstrncat(name, "help", n); ok=1;
	break;

    default:
	if (modified) {
	    if (isgraph(s->unicode)) {
		xstrnapp(name, s->unicode, n); ok=1;
	    }
	    else if (s->sym >= 33 && s->sym < 128) {
		xstrnapp(name, s->sym, n); ok=1;
	    }
	}
    }
    if (ok) {
	xstrncat(name, "\n", n);
	xstrncpy(buf, name, maxbuf);
    }
    return ok;
}

/* Get a string from a cdb panel and do special input processing. */
char *
sdlgr_gets(char *buf, int maxbuf, const char *prompt)
{
    SDL_Event ev;
    SDL_Event *e = &ev;
    int done = 0;			/* got input? */
    int hide = 0;			/* if true, user input is overriden
					   by a key/mouse macro */
    Cmd *curcmd = get_cmd_ring();
    int i;

    if (inputRedirection) {
	return fgets(buf, maxbuf, stdin);
    }

    /* Set prompt for current panel. */
    assert(prompt);
    set_prompt(TextPanels[curPanel-1], prompt);

    /* Reactivate text panels. */
    for (i=0; i<NTextPanels; i++) {
	activate(TextPanels[i]);
	refresh_layout(TextPanels[i]->layout);
    }

    SDL_WarpMouse(xkoord(curAddr), ykoord(curAddr));

    /* Process events until we have input to pass back. */
    while (!done) {
	TextInput *t;
	while (0==SDL_WaitEvent(e)) {}	/* Get an event, ignore errors. */
	t = TextPanels[curPanel-1];

	switch (e->type) {
	case SDL_MOUSEBUTTONDOWN:
	    /* Expand mouse button macros. */
	    switch (e->button.button) {
	    case 1: xstrncpy(buf, " m mousel\n", maxbuf); done=hide=1; break;
	    case 2: xstrncpy(buf, " m mousem\n", maxbuf); done=hide=1; break;
	    case 3: xstrncpy(buf, " m mouser\n", maxbuf); done=hide=1; break;
	    }
	    /* Accept mouse input and update curAddr only if the click
	     * is inside core. */
	    if (done) {
		int x = e->button.x;
		int y = e->button.y;
		if (x >= ArenaX && y >= ArenaY)
		{
		    int col = (x-ArenaX)/CellSize;
		    int row = (y-ArenaY)/CellSize;
		    int addr = col + row*CellsPerLine;
		    if (col < CellsPerLine && addr < (int)coreSize) {
			curAddr = addr;
		    } else {
			xstrncpy(buf, "", maxbuf);
			done = hide = 0;
		    }
		}
	    }
	    break;

	case SDL_KEYDOWN:
	    /* Handle CTRL-C, CTRL-BREAK, ALT-RETURN, etc. */
	    if (special_keyhandler(&e->key.keysym)) { break; }

	    /* Possibly a history key? */
	    if (any_set(e->key.keysym.mod, KMOD_SHIFT)) {
		if (e->key.keysym.sym == SDLK_UP) {	/* SHIFT-UP */
		    curcmd = curcmd->next;
		    reset_input(t, curcmd->cmd);
		    refresh_layout(t->layout);
		    continue;		/* processing events. */
		}
		if (e->key.keysym.sym == SDLK_DOWN) { /* SHIFT-DOWN */
		    curcmd = curcmd->prev;
		    reset_input(t, curcmd->cmd);
		    refresh_layout(t->layout);
		    continue;		/* processing events. */
		}
	    }

	    /* print_keysym(&e->key.keysym); */
	    normalise_keysym(&e->key.keysym);
	    /* Expand macro keys?  (F1, F2, ALT-K, etc.) */
	    if (macro_key(&e->key.keysym, buf, maxbuf)) {
		done = hide = 1;
	    }
	    else {
		int c;
		/* Key goes into the current text input panel. */
		c = conv_key(&e->key.keysym);
		switch (c) {
		case 0:			/* Unknown key, ignore. */
		    break;
		case '\b':		/* DEL, BACKSPACE erases a character.*/
		    del_char(t);
		    refresh_layout(t->layout);
		    break;
		case '\t':		/* TAB moves focus to next panel. */
		    take_focus(t);
		    refresh_layout(t->layout);
		    curPanel = 1 + (curPanel-1 + 1) % NTextPanels;
		    t = TextPanels[curPanel-1];
		    give_focus(t);
		    refresh_layout(t->layout);
		    break;
		case '\n':		/* RETURN, ENTER finishes input. */
		    add_char(t, c);
		    finish_input(t, buf, maxbuf);
		    done = 1;
		    break;
		default:		/* It's an ASCII char; add it. */
		    if (isgraph(c) || isspace(c)) {
			add_char(t, c);
			refresh_layout(t->layout);
			break;
		    }
		}
	    }
	    break;
	    /* The default handler handles window resize, quit, and
	       expose events */
	default:
	    default_handler(e);
	}
    }

    /* If user input was overriden by a key/mouse macro, show the
     * expansion text as if the user had input it (but don't forget
     * what the user was composing.)  Also, don't enter the expansion
     * text into the history ring.  */
    if (hide) {
	TextInput *t = TextPanels[curPanel-1];
	t->needs_prompting = 1;		/* Forces redraw of prompt and
					   the interrupted input line
					   next time the panel is activated.
					*/
	hide_input(t);
	type(t->out, buf, t->colix);
    } else {
	/* Add user input to the history ring if it's not empty. */
	char *s = buf;
	while (s && isspace(*s)) { s++; }
	if (s && *s!=0) {
	    add_cmd(buf);
	}
    }

    /* Deactivate the text panels so they don't redraw the prompt when
     * they are resized/exposed.  */
    for (i=0; i<NTextPanels; i++) {
	deactivate(TextPanels[i]);
	refresh_layout(TextPanels[i]->layout);
    }

    return buf;
}


/* ------------------------------------------------------------------------
 * Core display interface
 */

void
sdlgr_set_displayMode(int newmode)
{
    newmode = min(9, newmode);
    newmode = max(0, newmode);
    if (newmode != displayMode) {
	displayMode = newmode;
	if (TheSurf) {
	    sdlgr_relayout();
	}
    }
}

void
sdlgr_set_displaySpeed(int newspeed)
{
    newspeed = max(newspeed, 0);
    displaySpeed = min(newspeed, SPEEDLEVELS-1);
    keyDelay = keyDelayAr[displaySpeed];
    keyDelay = min(keyDelay, cycles>1 ? cycles-1 : 1);
    if (displayLevel == 0) {
	keyDelay = 10000;
    }
}

void
sdlgr_set_displayLevel(int newlevel)
{
    newlevel = min(4, newlevel);
    displayLevel = max(newlevel, 0);
    sdlgr_set_displaySpeed(displaySpeed);
}

static void
sdlgr_display_cycle()
{
    if (cycle % cycleRatio == 0) {
	Sint16 x = MetersX + cycle/cycleRatio;
	Slock(TheSurf);
	putpixel(TheSurf, x, cycleY, Colours[BLACK]);
	Sunlock(TheSurf);
	/*SDL_UpdateRect(TheSurf, x, cycleY, 1, 1);*/
    }

    /* If it's time to check for keys and refresh the display, do so. */
    if (cycle % keyDelay == 0) {
	SDL_Event e;
	int key = 0;

	/* Check for key presses and display changes. */
	if (SDL_PollEvent(&e)) {
	    switch (e.type) {
	    case SDL_KEYDOWN:
		if (!special_keyhandler(&e.key.keysym)) {
		    key = conv_key(&e.key.keysym);
		}
		break;
	    default:
		default_handler(&e);
	    }

	}
	if (key && !inputRedirection) {
	    switch (key) {
	    case ' ':
	    case 'r':
	    case 'R':
		sdlgr_clear_arena();
		break;

	    case '>':
		sdlgr_set_displaySpeed(displaySpeed-1);
		break;

	    case '<':
		sdlgr_set_displaySpeed(displaySpeed+1);
		break;

	    case 27:			/* ASCII for escape. */
	    case 'Q':
	    case 'q':
		e.type = SDL_QUIT;
		default_handler(&e);
		break;

	    case '0': sdlgr_set_displayLevel(0); break;
	    case '1': sdlgr_set_displayLevel(1); break;
	    case '2': sdlgr_set_displayLevel(2); break;
	    case '3': sdlgr_set_displayLevel(3); break;
	    case '4': sdlgr_set_displayLevel(4); break;

	    default:
		sighandler(0);	/* terminate macros in progress, enter
                                   debug state. */
		break;
	    }
	    sdlgr_write_menu();
	}
	refresh_layout(CoreLayout);
	refresh_layout(MetersLayout);
    }
}

#define display_die(warnum)
#define display_push(warnum)
#define display_init() sdlgr_open_graphics()
#define display_clear() sdlgr_display_clear()
#define display_close() sdlgr_display_close(WAIT)
#define display_cycle() sdlgr_display_cycle()

#define display_read(addr) \
    do {\
	if (displayLevel > 3)\
	    display_box((addr), TL, WarColourIx[W-warrior]);\
    } while (0)

#define display_write(addr) \
    do {\
	if (displayLevel > 1)\
	    display_box((addr), TR|BL, WarColourIx[W-warrior]);\
    } while (0)

#define display_dec(addr) \
    do {\
	if (displayLevel > 2)\
	    display_box((addr), TL|TR, WarColourIx[W-warrior]);\
    } while (0)

#define display_inc(addr) \
    do {\
	if (displayLevel > 2)\
	    display_box((addr), TL|BL, WarColourIx[W-warrior]);\
    } while (0)

#define display_exec(addr) \
    do {\
	if (displayLevel > 0)\
	    display_box((addr), TL|TR|BL|BR, WarColourIx[W-warrior]);\
    } while (0)

#define display_spl(warNum, tasks) \
    do {\
	int __w = (warNum);\
	Sint16 x = MetersX + (tasks)/processRatio;\
	Sint16 y = splY[__w];\
        Slock(TheSurf);\
	putpixel(TheSurf, x, y, Colours[WarColourIx[__w]]);\
        Sunlock(TheSurf);\
	/* SDL_UpdateRect(TheSurf, x, y, 1, 1); */\
    } while (0)

#define display_dat(addr, warNum, tasks) \
    do {\
	int __w = (warNum);\
	int x = MetersX + (tasks)/processRatio;\
	int y = splY[__w];\
	if (displayLevel > 0)\
	    display_box((addr), TL|TR|BL|BR, DieColourIx[__w]);\
        Slock(TheSurf);\
	putpixel(TheSurf, x, y, Colours[BLACK]);\
        Sunlock(TheSurf);\
	/* SDL_UpdateRect(TheSurf, x, y, 1, 1); */\
    } while (0)

/* ------------------------------------------------------------------------
 * Graphics display open/close.
 */

/* atexit() hook that quits SDL. */
static void
exit_hook(void)
{
    static int has_quit = 0;
    if (has_quit) { return; }

    has_quit = 1;
    if (TheSurf) {
	SDL_FreeSurface(TheSurf);
	TheSurf = NULL;
    }
    SDL_Quit();
}

/* Called by pMARS to initialise the SDL graphics subsystem. */
void
sdlgr_open_graphics(void)
{
    const char *modestr;

    /* Initialise SDL. */
    atexit(exit_hook);
    if (SDL_Init(SDL_INIT_VIDEO)) {
	exit_panic(failedSDLInit, SDL_GetError());
    }
    TheSurf = NULL;
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_EnableUNICODE(1);

    /* Decode -mode argument. */
    modestr = sdlgr_Storage[0] ? sdlgr_Storage[0] : "";
    if ((modestr = decode_vmode_string(&TheVMode, modestr))) {
	exit_panic(badModeString, modestr);
    }

    /* Preinitialise the font. (= clear it out with NULLs.)*/
    Font_Init(&CurrentFont, NULL, 8, 16, 128);

    /* Set up the layout. */
    init_layout();
    relayout(TheVMode.w, TheVMode.h);
}

/* Called by pMARS to close the graphics subsystem.  If WAIT is non-zero, then
 * prints a message waits for a keypress before shutting down. */
void
sdlgr_display_close(int wait)
{
    SDL_Event e;
    if (wait == WAIT) {
	sdlgr_puts(pressAnyKey);
	sdlgr_refresh(curPanel);
    }
    while (wait == WAIT) {
	if (SDL_WaitEvent(&e)) {
	    switch (e.type) {
	    case SDL_KEYDOWN:
		wait = NOWAIT; break;
	    default:
		default_handler(&e);
	    }
	}
    }
    exit_hook();
}
