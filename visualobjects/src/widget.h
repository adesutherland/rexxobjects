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
// File Name   : widget.h
// Description : C++ header file for the Widget Class
//             : All GUI items are subclasses of this abstract class
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

#ifndef VRORexx_widget_h
#define VRORexx_widget_h

#include <map>
#include <string>
#include <vector>
#include <queue>
#include <fltk/Widget.h>

#include "object.h"

// For requests for getInput from non-main threads
class GetInputRequest;

// Functions for external functions (to be called from rexx)
class VROWidget;
typedef int(*VROWIDGET_FUNCTION)(VROWidget* object, std::vector<std::string> &args, std::string &ret);

class VROWidget : public BROBase
{
 private:
  static bool WidgetInitted;
  static std::map<std::string, VROWIDGET_FUNCTION> VROWIDGET_FUNCTIONs;
  void VROWidget_setup();
  fltk::Widget* widget;
  static bool signalMainThread;
  static std::queue<VROWidget*> needShow;
  static std::queue<GetInputRequest*> needGetInput;

 protected:
  static void callbackHandler(fltk::Widget* w, void* data);
  void VROWidget_kill(bool propagate);
  VROWidget(BROBase* parent, std::string name);
  VROWidget(BROBase* parent, std::string name, std::vector<std::string> &args);
  void setWidget(fltk::Widget* _widget);
  fltk::Widget* getWidget();

 public:
  static int GuiEventLoop();
  static void InitGui();
  static void DoneWithGui();

  virtual int functionHandler(std::string name, std::vector<std::string> &args, std::string &ret);
  virtual int procedureHandler(std::string name, std::vector<std::string> &args);

  virtual ~VROWidget();

  static int staticSetExternalCallback(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void setExternalCallback(std::string rexxfunction);

  static int staticSetCallback(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void setCallbackWithStatic(const char* rexxcode);
  void setCallback(std::string& rexxcode);

  static int staticFlush(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  static void flush();

  static int staticHide(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void hide();

  static int staticShow(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void show();

  static int staticActivate(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void activate();

  static int staticDeactivate(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void deactivate();

  static int staticBoxType(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void boxType(fltk::Box* btype);

  static int staticLabelFont(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void labelFont(fltk::Font* font);

  static int staticLabelSize(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void labelSize(float size);

  static int staticLabelType(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  void labelType(fltk::LabelType* ltype);

  static int staticGetInput(VROWidget* object, std::vector<std::string> &args, std::string &ret);
  static void getInput(std::string prompt, std::string &entered);
};


// There is no factory for a widget class as we don't ever wont to make one
// directly - i.e. it is abstract

// Exceptions
class BROFailedToCreateGuiClass : public BROException
{
 public:
  BROFailedToCreateGuiClass(std::string message);
  BROFailedToCreateGuiClass();
};

#endif
