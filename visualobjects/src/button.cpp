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
// File Name   : button.cpp
// Description : Button Class
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

#include "button.h"

#include "debug.h"

#include <fltk/run.h>

// Button specific functiosn
std::map<std::string, VROBUTTON_FUNCTION> VROButton::VROBUTTON_FUNCTIONs;

// Common Constructor Functionality
void VROButton::VROButton_setup(int left, int top, int width, int height, char* title)
{
  type="VROButton";
  if (isChildThread()) fltk::lock();
  fltk::Button* button = new fltk::Button(left, top, width, height, title);
  if (isChildThread()) fltk::unlock();
  if (!button)
  {
    VROButton_kill(true);
    throw BROFailedToCreateGuiClass("fltk:Button");
  }
  setWidget(button);

  /* No specific functions yet ...
  if (VROBUTTON_FUNCTIONs.empty())
  {
    VROBUTTON_FUNCTIONs["XYZ"] = VROButton::staticXYZ;
  }
  */
};

// Constructor - always failed as we need some arguments
VROButton::VROButton(BROBase* parent, std::string name) : VROWidget(parent, name)
{
  VROButton_kill(true);
  throw BROInvalidConstructorArguments("Missing left, top, width, height, title");
}

// Constructor - called from a c++ client directly
VROButton::VROButton(BROBase* parent, std::string name, int left, int top, int width, int height, char* title) :
           VROWidget(parent, name)
{
  VROButton_setup(left, top, width, height, title);
}

// Constructor - called by factory/rexx
VROButton::VROButton(BROBase* parent, std::string name, std::vector<std::string> &args) :
           VROWidget(parent, name)
{
  int left, top, width, height;

  if (args.size()!=5)
  {
    VROButton_kill(true);
    throw BROInvalidConstructorArguments("5 Required - left, top, width, height, title");
  }

  left  = atoi(args[0].c_str()); // left==0 is valid

  top   = atoi(args[1].c_str()); // top==0 is valid

  width  = atoi(args[2].c_str());
  if (width==0)
  {
    VROButton_kill(true);
    throw BROInvalidConstructorArguments("Invalid width: " + args[0]);
  }

  height = atoi(args[3].c_str());
  if (height==0)
  {
    VROButton_kill(true);
    throw BROInvalidConstructorArguments("Invalid height: " + args[1]);
  }

  VROButton_setup(left, top, width, height, (char*)args[4].c_str());
}

// Common Destructor functionality
void VROButton::VROButton_kill(bool propagate)
{
  LOG("VROButton::VROButton_kill()");
  enterObjectCriticalSection();
  fltk::Button* button = (fltk::Button*)getWidget();
  if (button) delete button;
  setWidget(0);
  leaveObjectCriticalSection();

  if (propagate) VROWidget_kill(propagate);
  LOG("VROButton::VROButton_kill() - done");
};

// Destructor
VROButton::~VROButton()
{
  VROButton_kill(false); // No propagation as C++ does this for destructors automatically
};

/********************************************************************/
/* VROButtonFactory Functions                                       */
/********************************************************************/

// The [one and only] factory to make button objects
static VROButtonFactory factory;

// Constructor
VROButtonFactory::VROButtonFactory() : BROBaseFactory("VROButton") { };

// Factory function
BROBase* VROButtonFactory::newObject(BROBase* parent, std::string name)
{
  return new VROButton(parent, name);
};

BROBase* VROButtonFactory::newObject(BROBase* parent, std::string name, std::vector<std::string> &args)
{
  return new VROButton(parent, name, args);
};
