/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details:
 *
 *  http://www.gnu.org/copyleft/gpl.txt
 */

/*****************************************************************************/

#include "../common/common.h"
#include "../common/shared.h"
#include "../demodulator/pll.h"
#include "operation.h"
#include "utils.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*****************************************************************************/

static void sig_handler(int signal);

/*****************************************************************************/

int main(int argc, char *argv[]) {
    /* New and old actions for sigaction routine */
    struct sigaction sa_new, sa_old;

    /* Initialize new actions */
    sa_new.sa_handler = sig_handler;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;

    /* Register function to handle signals */
    sigaction(SIGINT,  &sa_new, &sa_old);
    sigaction(SIGSEGV, &sa_new, 0);
    sigaction(SIGFPE,  &sa_new, 0);
    sigaction(SIGTERM, &sa_new, 0);
    sigaction(SIGABRT, &sa_new, 0);
    sigaction(SIGCONT, &sa_new, 0);
    sigaction(SIGALRM, &sa_new, 0);

    /* Defaults/initialization */
    SetFlag(VERBOSE_MODE);
    rc_data.decode_timer = 0;
    snprintf(mlrpt_cfg, sizeof(mlrpt_cfg),
            "%s/.mlrptrc", getenv("HOME"));

    /* Process command line options */
    int option;

    while ((option = getopt(argc, argv, "c:s:qhiv")) != -1)
        switch (option) {
            case 'c': /* User-supplied config */
                strncpy(mlrpt_cfg, optarg, PATH_MAX + 1);

                break;

            case 's': /* Start and stop times as HHMM-HHMM in UTC */
                Auto_Timer_Setup(optarg);
                pause(); /* Pause here till start time */

                break;

            case 'q': /* Disable verbose output */
                ClearFlag(VERBOSE_MODE);

                break;

            case 'h': /* Print help and exit */
                Usage();
                exit(0);

                break;

            /* TODO should be renamed to not mess up our users (use flip instead) */
            case 'i': /* Enable image flipping (invert images) */
                SetFlag(IMAGE_INVERT);

                break;

            case 'v': /* Print version info and exit */
                puts(PACKAGE_STRING);
                exit(0);

                break;

            default: /* Print help and exit */
                Usage();
                exit(-1);

                break;
        }

    /* Prepare program cache directory */
    if (!prepareCacheDirectory()) {
        fprintf(stderr, "mlrpt: %s\n", "error during preparing cache directory");
        exit(-1);
    }

    /* Load configuration data */
    if (!loadConfig()) {
        Print_Message("Failed to load mlrpt configuration - exiting",
                ERROR_MESG);
        exit(-1);
    }

    /* Start receiver and decoder */
    /* TODO need more accurate names */
    if (!Start_Receiver()) {
        Print_Message("Failed to start Receiver and Decoder - exiting",
                ERROR_MESG);
        exit(-1);
    }

    return 0;
}

/*****************************************************************************/

/* sig_handler()
 *
 * Signal action handler function
 */
static void sig_handler(int signal) {
    if (signal == SIGALRM) {
        Alarm_Action();

        return;
    }

    /* Internal wakeup call */
    if (signal == SIGCONT) return;

    ClearFlag(STATUS_RECEIVING);
    fprintf(stderr, "\n");

    switch (signal) {
        case SIGINT:
            Print_Message("Exiting via User Interrupt", ERROR_MESG);

            if (rc_data.psk_mode == IDOQPSK)
                SetFlag(ACTION_IDOQPSK_STOP);
            else
                ClearFlag(ACTION_FLAGS_ALL);

            return;

            break;

        case SIGSEGV:
            Print_Message("Segmentation Fault - exiting", ERROR_MESG);
            exit(-1);

            break;

        case SIGFPE:
            Print_Message("Floating Point Exception - exiting", ERROR_MESG);
            exit(-1);

            break;

        case SIGABRT:
            Print_Message("Abort Signal received - exiting", ERROR_MESG);
            exit(-1);

            break;

        case SIGTERM:
            Print_Message("Termination Request received - exiting", ERROR_MESG);
            exit(-1);

            break;
    }
}
