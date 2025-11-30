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
#include "conf.h"
#include "local.h"
#include "logging.h"
#include "privops.h"
#include "sched.h"
#include "util.h"

/* Maximum frequency offset accepted by the kernel (in ppm) */
#define MAX_FREQ 500000.0

/* RTC synchronisation - once an hour */

static struct timespec last_rtc_sync;
#define RTC_SYNC_INTERVAL (60 * 60.0)

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

static void
synchronise_rtc(void)
{
  struct timespec ts, new_ts;
  struct timeval tv;
  double err;

  LCL_ReadRawTime(&ts);
  UTI_TimespecToTimeval(&ts, &tv);

  if (PRV_SetTime(&tv, NULL) < 0)
  {
    DEBUG_LOG("settimeofday() failed");
    return;
  }

  LCL_ReadRawTime(&new_ts);
  err = UTI_DiffTimespecsToDouble(&new_ts, &ts);

  lcl_InvokeDispersionNotifyHandlers(fabs(err));
}

/* ================================================== */

static void
set_sync_status(int synchronised, double est_error, double max_error)
{
  if (!synchronised)
    return;

  if (CNF_GetRtcSync()) {
    struct timespec now;
    double rtc_sync_elapsed;

    SCH_GetLastEventTime(NULL, NULL, &now);
    rtc_sync_elapsed = UTI_DiffTimespecsToDouble(&now, &last_rtc_sync);
    if (fabs(rtc_sync_elapsed) >= RTC_SYNC_INTERVAL) {
      synchronise_rtc();
      last_rtc_sync = now;
      DEBUG_LOG("rtc synchronised");
    }
  }
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

  LCL_ReadRawTime(&last_rtc_sync);

  SYS_Generic_CompleteFreqDriver(MAX_FREQ, 1.0 / cinfo.hz,
                                 read_frequency, set_frequency, NULL,
                                 0.0, 0.0,
                                 NULL, NULL,
                                 NULL, set_sync_status);
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
