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
// File Name   : console.cpp
// Description : Console Window
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

#include <iostream>                     /* needed for printf()        */

#include <fltk/events.h>
#include <fltk/run.h>

#include "console.h"
#include "debug.h"

using namespace fltk;

// Style Table
TextDisplay::StyleTableEntry ConsoleWindow::styleTable[] = {
  { BLACK,  COURIER, 12, 0 },   // A - StdOut
  { RED,    COURIER, 12, 0 },   // B - StdErr
  { BLUE,   COURIER, 12, 0 },   // C - StdIn
  { 60,     COURIER, 12, 0 }    // D - Trace - (Note: 60 = DARK_GREEN)
};


int ConsoleWindow::styleTableSize = 4;


ConsoleWindow::ConsoleWindow() :
     VROForm(0, "console", 500, 400, "Visual RexxObjects Console")
{
    beginNewObject();
    if (isChildThread()) fltk::lock();
    fltk::Window* window = (fltk::Window*)getWidget();

    buffer = new TextBuffer();
    highlight = new TextBuffer();

    output = new TextDisplay(20, 20, 500-40, 400-40);

    output->highlight_data(highlight, styleTable, styleTableSize, 0, NULL, NULL);

    window->callback(WindowCallback,this);
    output->tooltip("This shows the REXX console output");
    output->buffer(buffer);
    window->resizable(output);

    if (isChildThread()) fltk::unlock();
    endNewObject();
};


ConsoleWindow::~ConsoleWindow()
{
    enterObjectCriticalSection();
    delete output;
    delete buffer;
    delete highlight;
    buffer=0; output=0; output=0;
    leaveObjectCriticalSection();
};


void ConsoleWindow::pLineOut(std::string text, char high)
{
  std::string out("> ");
  out = out + text;
  // 
  
  
  out = out + text + "\n";
  pCharsOut(out, high);
};


void ConsoleWindow::pCharsOut(std::string text, char high)
{
  LOG("ConsoleWindow::pCharsOut()");
  IFLOG(isChildThread(),"ConsoleWindow::pCharsOut() - Child Thread!");

  show();
  enterObjectCriticalSection();
  if (!buffer || !output)
  {
    leaveObjectCriticalSection();
    return;
  }
  if (isChildThread()) fltk::lock();
  std::string h;
  h.assign(text.size(), high );
  highlight->append(h.c_str());

  buffer->append(text.c_str());
  output->move_down();
  output->show_insert_position();
  if (isChildThread()) fltk::unlock();
  leaveObjectCriticalSection();
  flush();
};


void ConsoleWindow::pWindowCallback() {
    hide();
};


void ConsoleWindow::WindowCallback(Widget*, void* v) {
    ((ConsoleWindow*)v)->pWindowCallback();
};


void ConsoleWindow::pGetInput(std::string &entered) {
  IFLOG(isChildThread(),"ConsoleWindow::pGetInput() - Child Thread!");
  show();

  VROWidget::getInput("Console Input Required", entered);

  std::string out("< ");
  out = out + entered + "\n";
  pCharsOut(out, 'C');
}


void ConsoleWindow::LineOut(std::string text)
{
  LOG("ConsoleWindow::LineOut():" + text)
  ((ConsoleWindow*)consoleIO)->pLineOut(text, 'A');
}


void ConsoleWindow::ErrorOut(std::string text)
{
  ((ConsoleWindow*)consoleIO)->pLineOut(text, 'B');
}


void ConsoleWindow::TraceOut(std::string text)
{
  ((ConsoleWindow*)consoleIO)->pLineOut(text, 'D');
}


void ConsoleWindow::GetInput(std::string &entered)
{
  ((ConsoleWindow*)consoleIO)->pGetInput(entered);
}
