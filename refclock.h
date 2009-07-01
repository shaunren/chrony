/*
  chronyd/chronyc - Programs for keeping computer clocks accurate.

 **********************************************************************
 * Copyright (C) Richard P. Curnow  2009
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
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 * 
 **********************************************************************

  =======================================================================

  Header file for refclocks.

  */

#ifndef GOT_REFCLOCK_H
#define GOT_REFCLOCK_H

#include "srcparams.h"
#include "sources.h"

typedef struct {
  char driver_name[4];
  int driver_parameter;
  int driver_poll;
  int poll;
  int filter_length;
  unsigned long ref_id;
  double offset;
} RefclockParameters;

typedef struct RCL_Instance_Record *RCL_Instance;

typedef struct {
  int (*init)(RCL_Instance instance);
  void (*fini)(RCL_Instance instance);
  int (*poll)(RCL_Instance instance);
} RefclockDriver;

extern void RCL_Initialise(void);
extern void RCL_Finalise(void);
extern int RCL_AddRefclock(RefclockParameters *params);
extern void RCL_StartRefclocks(void);
extern void RCL_StartRefclocks(void);
extern void RCL_ReportSource(RPT_SourceReport *report, struct timeval *now);

/* functions used by drivers */
extern void RCL_SetDriverData(RCL_Instance instance, void *data);
extern void *RCL_GetDriverData(RCL_Instance instance);
extern int RCL_GetDriverParameter(RCL_Instance instance);
extern int RCL_AddSample(RCL_Instance instance, struct timeval *sample_time, double offset, NTP_Leap leap_status);

#endif
