//                                        -*- C++ -*-
/*

Copyright (C) 1992, 1993, 1994, 1995 John W. Eaton

This file is part of Octave.

Octave is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

Octave is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, write to the Free
Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#if defined (__GNUG__)
#pragma implementation
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <float.h>

#include "dbleDET.h"

int
DET::value_will_overflow (void) const
{
  return det[2] + 1 > log10 (DBL_MAX) ? 1 : 0;
}

int
DET::value_will_underflow (void) const
{
  return det[2] - 1 < log10 (DBL_MIN) ? 1 : 0;
}

double
DET::coefficient (void) const
{
  return det[0];
}

int
DET::exponent (void) const
{
  return (int) det[1];
}

double
DET::value (void) const
{
  return det[0] * pow (10.0, det[1]);
}

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; page-delimiter: "^/\\*" ***
;;; End: ***
*/
