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
// File Name   : form.cpp
// Description : Form Class
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

#include "debug.h"

#include "form.h"

#include <fltk/run.h>

// Form specific functiosn
std::map<std::string, VROFORM_FUNCTION> VROForm::VROFORM_FUNCTIONs;


// Common Constructor Functionality
void VROForm::VROForm_setup(int width, int height, char* title)
{
  LOG("VROForm::VROForm_setup");
  type="VROForm";
  if (isChildThread()) fltk::lock();
  fltk::Window*  window = new fltk::Window(width, height, title);
  if (isChildThread()) fltk::unlock();
  if (!window)
  {
    LOG("VROForm::VROForm_setup ... window not created");
    VROForm_kill(true);
    throw BROFailedToCreateGuiClass("fltk:Window");
  }
  setWidget(window);

  /* No specific functions yet ...
  if (VROFORM_FUNCTIONs.empty())
  {
    VROFORM_FUNCTIONs["XYZ"] = VROForm::staticXYZ;
  }
  */
};


// Constructor - always failed as we need some arguments
VROForm::VROForm(BROBase* parent, std::string name) : VROWidget(parent, name)
{
  LOG("VROForm::VROForm(parent, name)");
  VROForm_kill(true);
  throw BROInvalidConstructorArguments("Missing width, height, title");
}


// Constructor - called from a c++ client directly
VROForm::VROForm(BROBase* parent, std::string name, int width, int height, char* title) :
           VROWidget(parent, name)
{
  LOG("VROForm::VROForm(parent, name, width, height, title)");
  VROForm_setup(width, height, title);
}


// Constructor - called by factory/rexx
VROForm::VROForm(BROBase* parent, std::string name, std::vector<std::string> &args) :
           VROWidget(parent, name)
{
  LOG("VROForm::VROForm(parent, name, args)");

  int width, height;

  if (args.size()!=3)
  {
    VROForm_kill(true);
    throw BROInvalidConstructorArguments("3 Required - width, height, title");
  }
  width  = atoi(args[0].c_str());
  if (width==0)
  {
    VROForm_kill(true);
    throw BROInvalidConstructorArguments("Invalid width: " + args[0]);
  }
  height = atoi(args[1].c_str());
  if (height==0)
  {
    VROForm_kill(true);
    throw BROInvalidConstructorArguments("Invalid height: " + args[1]);
  }

  VROForm_setup(width, height, (char*)args[2].c_str());
}


// Common Destructor functionality
void VROForm::VROForm_kill(bool propagate)
{
  enterObjectCriticalSection();
  fltk::Window* window = getWindow();
  if (window)
  {
    window->remove_all();
    delete window;
  }
  setWidget(0);
  leaveObjectCriticalSection();
  if (propagate) VROWidget_kill(propagate);
};


// Destructor
VROForm::~VROForm()
{
  VROForm_kill(false); // No propagation as C++ does this for destructors automatically
};


void VROForm::beginNewObject()
{
  fltk::Window* window = getWindow();
  if (window)
  {
    if (isChildThread()) fltk::lock();
    window->begin();
    if (isChildThread()) fltk::unlock();
  }
};


void VROForm::endNewObject()
{
  fltk::Window* window = getWindow();
  if (window)
  {
    if (isChildThread()) fltk::lock();
    window->end();
    if (isChildThread()) fltk::unlock();
  }
};


/********************************************************************/
/* VROFormFactory Functions                                         */
/********************************************************************/

// The [one and only] factory to make form objects
static VROFormFactory factory;

// Constructor
VROFormFactory::VROFormFactory() : BROBaseFactory("VROForm") { };

// Factory function
BROBase* VROFormFactory::newObject(BROBase* parent, std::string name)
{
  return new VROForm(parent, name);
};

BROBase* VROFormFactory::newObject(BROBase* parent, std::string name, std::vector<std::string> &args)
{
  return new VROForm(parent, name, args);
};
