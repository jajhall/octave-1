//                                  -*- C++ -*-
/*

Copyright (C) 1996 John W. Eaton

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
Software Foundation, 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#if !defined (octave_ComplexQR_h)
#define octave_ComplexQR_h 1

#if defined (__GNUG__)
#pragma interface
#endif

class ostream;

#include "CMatrix.h"
#include "dbleQR.h"

class
ComplexQR
{
public:

  ComplexQR (void) : q (), r () { }

  ComplexQR (const ComplexMatrix& A, QR::type qr_type = QR::std);

  ComplexQR (const ComplexQR& a) : q (a.q), r (a.r) { }

  ComplexQR& operator = (const ComplexQR& a)
    {
      if (this != &a)
	{
	  q = a.q;
	  r = a.r;
	}
      return *this;
    }

  ComplexMatrix Q (void) const { return q; }
  ComplexMatrix R (void) const { return r; }

  friend ostream&  operator << (ostream& os, const ComplexQR& a);

protected:

  ComplexMatrix q;
  ComplexMatrix r;
};

#endif

/*
;;; Local Variables: ***
;;; mode: C++ ***
;;; page-delimiter: "^/\\*" ***
;;; End: ***
*/
