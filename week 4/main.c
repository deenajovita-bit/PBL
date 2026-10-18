/*
 * main.c  (Team Leader)
 * Entry point of the Deadlock Simulation System.
 * - system_state(): tells if the system is SAFE, UNSAFE or DEADLOCKED
 * - main(): creates the system and starts the GUI
 */

#include "deadlock.h"

/*
 * Decide the state of the system.
 * Order of checking: DEADLOCK first, then UNSAFE, then SAFE.
 */
SysState system_state(System *s)
{
    int dead[MAX_P];
    int seq[MAX_P];

    /* Make sure Available and Need are up to date */
    update_available(s);
    calc_need(s);

    /* 1. Some processes are stuck waiting for each other */
    if (detect_deadlock(s, dead) > 0)
        return STATE_DEADLOCK;

    /* 2. No deadlock yet: check if a safe sequence exists */
    if (is_safe(s, seq))
        return STATE_SAFE;

    /* 3. No safe sequence */
    return STATE_UNSAFE;
}

int main(int argc, char **argv)
{
    System sys;

    /* Start with 3 processes, 3 resource types.
       Total instances: R0 = 3, R1 = 3, R2 = 2 */
    int total[3] = {3, 3, 2};
    sys_init(&sys, 3, 3, total);

    calc_need(&sys);

    /* Open the GTK window. Runs until the user closes it. */
    return gui_run(&sys, argc, argv);
}
