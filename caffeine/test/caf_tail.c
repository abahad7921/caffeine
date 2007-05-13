/* -*- mode: c; c-default-style: "k&r"; indent-tabs-mode: nil; -*- */
/* vim:set ft=c ff=unix ts=4 sw=4 enc=latin1 expandtab: */
/* kate: space-indent on; indent-width 4; mixedindent off; indent-mode none; */
/*
    Caffeine - C Application Framework
    Copyright (C) 2006 Daniel Molina Wegener

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "caf/caf.h"
#include "caf/caf_data_mem.h"
#include "caf/caf_data_buffer.h"

#ifdef BSD_SYSTEM
#define IO_EVENT_USE_KEVENT 1
#endif /* !BSD_SYSTEM */

#ifdef LINUX_SYSTEM
#define IO_EVENT_USE_INOTIFY 1
#endif /* !LINUX_SYSTEM */

#include "caf/caf_evt_fio.h"
#include "caf/caf_io_tail.h"

void sigrtn (int sig);
int tailrtn (cbuffer_t *b, int *ops);
int ctrl = CAF_OK;

int
main (int argc, char **argv)
{
    ctrl = CAF_OK;
    if (argc > 1) {
        printf ("opening %s\n", argv[1]);
        signal (SIGHUP, sigrtn);
        return 1;
    }
    return 0;
}

void
sigrtn (int sig)
{
    ctrl = CAF_ERROR;
    printf ("signal: %d\n", sig);
}

/** }@ */
/* caf_pidfile.c ends here */
