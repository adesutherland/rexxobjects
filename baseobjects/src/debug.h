// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : debug.h
// Description : Misc. Debug Macros
// *************************************************************************
// L I C E N S E
// *************************************************************************
// This program is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU General Public License as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// For the avoidance of doubt:
// - Version 3 of the license (i.e. not earlier nor later versions) apply.
// - a copy of the license text should be in the "license" directory of the
//   source distribution.
// - Requests for use under other licenses will be treated sympathetically,
//   please see contact details.
// *************************************************************************
// C O N T A C T   D E T A I L S
// *************************************************************************
// E-mail      : adrian@sutherlandonline.org
//             : adrian@open-bpm.org
//             : sutherland@users.sourceforge.net
// Web         : http://rexxobjects.sourceforge.net/
//             : www.open-bpm.org
// Telephone   : Please e-mail for details
// Postal      : UK - Please e-mail for details
// *************************************************************************

#ifndef ro_bo_debug_h
#define ro_bo_debug_h
#ifdef DEBUG

/* #include "console.h" */
#include <iostream>
/* #define LOG(x) {std::cerr << std::string((x)) << std::endl << std::flush; ConsoleWindow::TraceOut((x));} */
#define LOG(x) {std::cout << " - - - > " << std::string((x)) << std::endl << std::flush;}
#define LOGF(f,x) {f;LOG(x);}
#define IFLOG(c,t) {if (c) LOG(t);}
#define IFLOGF(c,tf, t) {if (c) {tf; LOG(t)};}
#define IFELSELOG(c,t,f) {if (c) LOG(t) else LOG(f)}
#define IFELSELOGF(c,tf,t,ff,f) {if (c) {tf; LOG(t)} else {ff; LOG(f)};}

#else

#define LOG(x)
#define LOGF(f,x)
#define IFLOG(c,t)
#define IFLOGF(c,tf, t)
#define IFELSELOG(c,t,f)
#define IFELSELOGF(c,tf,t,ff,f)

#endif
#endif
