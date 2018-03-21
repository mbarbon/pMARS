/* pMARS Globals:

   int displayLevel     0: no detail, 1: executions/deaths only
                        2: as level 1 but also writes, 3: as level 2 but also
                        decrements/increments, 4: as level 3 but also reads.

   int displaySpeed     0..SPEEDLEVELS-1.  0 is slowest, SPEEDLEVELS-1 is
                        fastest.  Speed level 1 is the default.

   int displayMode      Implementation dependent.  In windowed systems
                        typically controls the window size and size in pixels
                        of a core cell.  On non-windowed systems it
                        controls the display resolution.  Conventionally
                        display mode 0 is the same as display mode 1.

   int curPanel         The current text panel, either 1 (left panel) or 2
                        (right panel) or -1 (none, not initialised).
                        cdb.c uses the function XXX_update(int nextpanel)
                        to update the panel and panel movement, so
                        only the display code needs to know about curPanel.

   int curAddr          While debugging this holds the current core address.
                        Managed by cdb.c --- read-only in the display code.

   int inCbd            A flag that tells the display code if we are debugging
                        or not.  Managed by cdb.c --- read-only in the display
                        code.

   int inputRedirection A flag that tells us whether or not input is
                        redirected from stdin.  read-only.

   void sighandler(int)  The signal handler function of pMARS.  Call this
                        with the argument 0 when suspending a fight to resume
                        debugging so that cdb knows to stop executing the
                        current macro.  Note: you need to add a test
                        to pmars.c to include <signal.h> if you call this.
 */
extern int displayLevel;
extern int displayMode;
extern int displaySpeed;
extern int curPanel;
extern int curAddr;
extern int inCdb;
extern int inputRedirection;
void sighandler(int);

/* Initialise the graphics subsystem. */
void
stdio_open_graphics(void)
{}

/* Close down the graphics subsystem.  If the argument WAIT is equal to
   the #define WAIT then print the message pressAnyKey and wait for a key
   press before closing down. */
void
stdio_display_close(int wait)
{}

/* Clear the arena. */
void
stdio_clear_arena(void)
{}

/* Clear the arena process meters and cycle meter. */
void
stdio_display_clear(void)
{}

/* Draw the status line.  That is, the line under the arena
   that has the speed meter, detail level indicator and debugging status.
 */
void
stdio_write_menu(void)
{}

/* Update the text panels.  If NEXTPANEL = 0 then close the current panel
 * curPanel.  If NEXTPANEL = 1 the update the cursor to the left panel.
 * If NEXTPANEL = 2 then update the cursor to the right panel, possibly
 * creating it if it didn't exist before.  On entry if curPanel is -1
 * then no panels have been initialised yet.
 */
void
stdio_update(int nextpanel)
{
	/* stdio doesn't support panels -- do nothing but update curPanel. */
	if (curPanel <= 0) {
		curPanel = 1;
	}
	if (nextpanel == 0) {
		curPanel = 1;
	} else {
		curPanel = nextpanel;
	}
}

/* Clear the current panel and move the cursor to the upper left corner
   of the current panel. */
void
stdio_clear(void)
{}

/* Print a string to the current panel.  The only special control character
   that needs to be recognised is '\n' that performs a line feed. */
void
stdio_puts(const char *s)
{
	printf(s);
}

/* Get the maximum number of visible rows in the current panel. */
int
stdio_text_lines(void)
{
	return 25;
}

/* Get a string from the current panel into the buffer BUF of at most
   MAXBUF-1 characters using the prompt string PROMPT.  Return BUF.
   As a special case, if the global inputRedirection is non-zero then
   read a line from the standard input stream.
 */
char *
stdio_gets(char *buf, int maxbuf, const char *prompt)
{
	stdio_puts(prompt);
	if (maxbuf > 0) {
		buf[maxbuf-1] = 0;
		return fgets(buf, maxbuf-1, stdin);
	}
	return buf; // or NULL?
}

/* Set the display mode to NEWMODE. */
void
stdio_set_displayMode(int newmode)
{
	newmode = newmode < 0 ? 0 : newmode;
	newmode = newmode > 9 ? 9 : newmode;
	displayMode = newmode;
}

/* Set the display speed to NEWSPEED. */
void
stdio_set_displaySpeed(int newspeed)
{
	newspeed = newspeed < 0 ? 0 : newspeed;
	newspeed = newspeed > SPEEDLEVELS-1 ? SPEEDLEVELS-1 : newspeed;
	displaySpeed = newspeed;
}

/* Set the display detail level to NEWLEVEL. */
void
stdio_set_displayLevel(int newlevel)
{
	newlevel = newlevel < 0 ? 0 : newlevel;
	newlevel = newlevel > 4 ? 4 : newlevel;
	displayLevel = newlevel;
}


/* --------------------------------------------------------------------------
 * Macros called from the simulator during execution.
 */

/* Called every cycle from the simulator to update the cycle meter and
   process special key strokes if the global flag inputRedirection
   is false.  The keys recognised should be:
 	SPACE r R 	Clear the arena.
	>		Speed up the display (decrement displaySpeed.)
	<		Slow down the display (increment displaySpeed.)
	ESCAPE q Q	Quit pMARS.
	0 1 2 3 4	Set the display detail level to 0 1 2 3 or 4.
	otherwise	Enter debugging state by calling sighandler(0).
 */
static void
stdio_display_cycle(void)
{}

/* The following display_XXX macros are expanded inside sim.c/simulator(). */

#define display_cycle() stdio_display_cycle()
#define display_close() stdio_display_close(WAIT)
#define display_init() stdio_open_graphics()
#define display_clear() stdio_display_clear()

/* All processes of the WARNUMth warrior died. */
#define display_die(warnum)

/* The given core address was pushed to the process queue of the current
   warrior.  Doesn't need to update the process meters.  */
#define display_push(addr)

/* Display the foo at the given core address if displayLevel is sufficient. */
#define display_read(addr)
#define display_write(addr)
#define display_dec(addr)
#define display_inc(addr)
#define display_exec(addr)

/* The WARNUMth warrior just split and now has NPROCS processes.  Update its
   process meter.  Called when an SPL instruction executes succesfully. */
#define display_spl(warnum, nprocs)

/* A process of the WARNUMth warrior executed a DAT at the core address ADDR
   and now has NPROCS processes left.  Display the death at ADDR if
   displayLevel is sufficient and update the warrior's process meter. */
#define display_dat(addr, warnum, nprocs)
