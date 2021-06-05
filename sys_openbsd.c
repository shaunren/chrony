/*
  chronyd/chronyc - Programs for keeping computer clocks accurate.

 **********************************************************************
 * Copyright (C) Richard P. Curnow  1997-2001
 * Copyright (C) J. Hannken-Illjes  2001
 * Copyright (C) Miroslav Lichvar  2015
 * Copyright (C) Shaun Ren  2021
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 **********************************************************************

  =======================================================================

  Driver file for the OpenBSD operating system.
  */

#include "config.h"

#include "sysincl.h"

#include "sys_generic.h"
#include "sys_openbsd.h"
#include "logging.h"
#include "privops.h"
#include "util.h"

/* Maximum frequency offset accepted by the kernel (in ppm) */
#define MAX_FREQ 500000.0

/* ================================================== */

static double
read_frequency(void)
{
  int64_t freq;

  if (PRV_AdjustFreq(NULL, &freq))
    LOG_FATAL("adjfreq() failed");

  return (double)-freq / (1000LL << 32);
}

/* ================================================== */

static double
set_frequency(double freq_ppm)
{
  int64_t freq;

  freq = -freq_ppm * (1000LL << 32);
  if (PRV_AdjustFreq(&freq, NULL))
    LOG_FATAL("adjfreq() failed");

  return read_frequency();
}

/* ================================================== */

static struct clockinfo
get_clockinfo(void)
{
  struct clockinfo cinfo;
  size_t cinfo_len;
  int mib[2];

  cinfo_len = sizeof(cinfo);
  mib[0] = CTL_KERN;
  mib[1] = KERN_CLOCKRATE;

  if (sysctl(mib, 2, &cinfo, &cinfo_len, NULL, 0) == -1)
    LOG_FATAL("sysctl() failed");

  return cinfo;
}

/* ================================================== */

static void
reset_adjtime_offset(void)
{
  struct timeval delta;
  
  memset(&delta, 0, sizeof(delta));

  if (PRV_AdjustTime(&delta, NULL))
    LOG_FATAL("adjtime() failed");
}


/* ================================================== */

void
SYS_OpenBSD_Initialise(void)
{
  struct clockinfo cinfo;

  cinfo = get_clockinfo();
  reset_adjtime_offset();

  SYS_Generic_CompleteFreqDriver(MAX_FREQ, 1.0 / cinfo.hz,
                                 read_frequency, set_frequency, NULL,
                                 0.0, 0.0,
                                 NULL, NULL,
                                 NULL, NULL);
}

/* ================================================== */

void
SYS_OpenBSD_Finalise(void)
{
  SYS_Generic_Finalise();
}

/* ================================================== */

#ifdef FEAT_PRIVDROP
void
SYS_OpenBSD_DropRoot(uid_t uid, gid_t gid, SYS_ProcessContext context, int clock_control)
{
  if (context == SYS_MAIN_PROCESS)
    PRV_StartHelper();

  UTI_DropRoot(uid, gid);

  if (pledge("stdio rpath wpath cpath unix inet dns settime", NULL) == -1)
    LOG_FATAL("pledge() failed");
}
#endif
