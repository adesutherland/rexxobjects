// *************************************************************************
// A B O U T   T H I S   W O R K  -  V I S U A L O B J E C T S
// *************************************************************************
// Work Name   : VisualObjects
// Description : Part of RexxObjects - Visual C++ objects for REXX based on
//             : FLTK.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : console.h
// Description : C++ header file for the console window
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

#ifndef VRORexx_console_h
#define VRORexx_console_h

#include "form.h"

#include <fltk/TextDisplay.h>

#include <string>

using namespace fltk;

class ConsoleWindow : public VROForm, public BROConsoleIO {
  TextBuffer* buffer;
  TextBuffer* highlight;
  TextDisplay* output;

  inline void pWindowCallback();
  static void WindowCallback(Widget*, void* v);
  void pLineOut(std::string text, char style);
  void pCharsOut(std::string text, char style);
  void pGetInput(std::string &entered);
  static TextDisplay::StyleTableEntry styleTable[];
  static int styleTableSize;

 protected:
  virtual void LineOut(std::string text);
  virtual void ErrorOut(std::string text);
  virtual void TraceOut(std::string text);
  virtual void GetInput(std::string &entered);

 public:
  ConsoleWindow();
  virtual ~ConsoleWindow();
};

#endif
