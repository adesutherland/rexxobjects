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
// File Name   : label.h
// Description : C++ header file for the Label Class
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

#ifndef VRORexx_label_h
#define VRORexx_label_h

#include "widget.h"

#include <map>
#include <string>
#include <vector>
#include <fltk/Widget.h>

class VROLabel;
typedef int(*VROLABEL_FUNCTION)(VROLabel* object, std::vector<std::string> &args, std::string &ret);

class VROLabelFactory;

class VROLabel : public VROWidget
{
 friend class VROLabelFactory;
 private:
  static std::map<std::string, VROLABEL_FUNCTION> VROLABEL_FUNCTIONs;
  void VROLabel_setup(int left, int top, int width, int height, char* title);

 protected:
  void VROLabel_kill(bool propagate);
  VROLabel(BROBase* parent, std::string name);
  VROLabel(BROBase* parent, std::string name, std::vector<std::string> &args);

 public:
  VROLabel(BROBase* parent, std::string name, int left, int top, int width, int height, char* title);
  virtual ~VROLabel();
};

// Factory Class for making label objects
class VROLabelFactory : public BROBaseFactory {
 public:
  VROLabelFactory();
  BROBase* newObject(BROBase* parent, std::string name);
  BROBase* newObject(BROBase* parent, std::string name, std::vector<std::string> &args);
};

#endif
