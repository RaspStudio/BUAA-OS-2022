/* See COPYRIGHT for copyright information. */

/* Timer is linked to IRQ4 (By GXEmul Preset) */

#include <kclock.h>

extern void set_timer();

/**	Enable Timer Int, Ready to enter User space;
 */
void kclock_init(void)
{
	set_timer();
}

