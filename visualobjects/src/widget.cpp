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
// File Name   : widget.cpp
// Description : Base GUI Widget Class
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
#include "widget.h"
#include "console.h"
#include <fltk/events.h>
#include <fltk/run.h>
#include <fltk/ask.h>
#include <stdlib.h>                    /* for atof() */

#ifdef WIN32
 #include <windows.h>
#else
 #include <pthread.h>
 #include <errno.h>
#endif

// For requests for getInput from non-main threads
class GetInputRequest
{
  public:
   std::string prompt;
   std::string result;
   #ifdef WIN32
    HANDLE requestDone;
   #else
    pthread_cond_t requestDone;
    pthread_mutex_t requestDoneMutex;
   #endif
};

std::map<std::string, VROWIDGET_FUNCTION> VROWidget::VROWIDGET_FUNCTIONs;

std::queue<VROWidget*> VROWidget::needShow;
std::queue<GetInputRequest*> VROWidget::needGetInput;

bool VROWidget::WidgetInitted = false;
bool VROWidget::signalMainThread = false;

// Event Loop Function
int VROWidget::GuiEventLoop()
{
  int rc = 0;
  if (isChildThread()) return -1;

  // rc = fltk::run();
  while (fltk::Window::first())
  {
    // LOG("VROWidget::GuiEventLoop() - Calling wait()");
    fltk::wait();
    // LOG("VROWidget::GuiEventLoop() - wait() done");

    if (signalMainThread)
    {
      LOG("VROWidget::GuiEventLoop() - Main thread signalled");
      enterStaticCriticalSection();
      signalMainThread = false;
      leaveStaticCriticalSection();

      // Process Show requests
#ifdef WIN32
      enterStaticCriticalSection();
      while( !needShow.empty() ) {
        needShow.front()->show();
        needShow.pop();
      }
      leaveStaticCriticalSection();
#endif

      // Process GetInput requests
      enterStaticCriticalSection();
      while( !needGetInput.empty() ) {
        GetInputRequest* request = needGetInput.front();
        needGetInput.pop();
        leaveStaticCriticalSection();
        getInput(request->prompt, request->result);
#ifdef WIN32
        SetEvent(request->requestDone);
#else
        pthread_mutex_lock(&(request->requestDoneMutex));
        pthread_cond_signal(&(request->requestDone));
        pthread_mutex_unlock(&(request->requestDoneMutex));
#endif
        enterStaticCriticalSection();
      }
      leaveStaticCriticalSection();
    }

  }
  return rc;
}


// Setup Gui
void VROWidget::InitGui()
{
  fltk::lock();
  if (!consoleIO) consoleIO = new ConsoleWindow;
  InitObject();
};


// Finished with Gui
void VROWidget::DoneWithGui()
{
  if (consoleIO)
  {
      delete consoleIO;
      consoleIO = 0;
  }
  fltk::unlock();
  DoneWithObject();
};


// Common Constructor Functionality
void VROWidget::VROWidget_setup()
{
  type="VROWidget";
  widget = 0;

  if (WidgetInitted == false)
  {
    enterStaticCriticalSection();
    if (WidgetInitted == false)
    {
      WidgetInitted = true;
      if (VROWIDGET_FUNCTIONs.empty())
      {
        VROWIDGET_FUNCTIONs["SETEXTERNALCALLBACK"] = VROWidget::staticSetExternalCallback;
        VROWIDGET_FUNCTIONs["SETCALLBACK"] = VROWidget::staticSetCallback;
        VROWIDGET_FUNCTIONs["HIDE"] = VROWidget::staticHide;
        VROWIDGET_FUNCTIONs["SHOW"] = VROWidget::staticShow;
        VROWIDGET_FUNCTIONs["ACTIVATE"] = VROWidget::staticActivate;
        VROWIDGET_FUNCTIONs["DEACTIVATE"] = VROWidget::staticDeactivate;
        VROWIDGET_FUNCTIONs["BOXTYPE"] = VROWidget::staticBoxType;
        VROWIDGET_FUNCTIONs["LABELFONT"] = VROWidget::staticLabelFont;
        VROWIDGET_FUNCTIONs["LABELSIZE"] = VROWidget::staticLabelSize;
        VROWIDGET_FUNCTIONs["LABELTYPE"] = VROWidget::staticLabelType;
        VROWIDGET_FUNCTIONs["GETINPUT"] = VROWidget::staticGetInput;
      }
    }
    leaveStaticCriticalSection();
  }
};


// Constructor
VROWidget::VROWidget(BROBase* parent, std::string name) : BROBase(parent, name)
{
  VROWidget_setup();
};


// Constructor
VROWidget::VROWidget(BROBase* parent, std::string name, std::vector<std::string> &args) :
           BROBase(parent, name, args)
{
  VROWidget_setup();
}


// Common Destructor Functionality
void VROWidget::VROWidget_kill(bool propagate)
{
  LOGF(std::string nm; getName(nm),"VROWidget::VROWidget_kill() name=" + nm + " type=" + type);

  enterObjectCriticalSection();
  widget = 0; // Note: subclasses are responsible for creating/deleting classes widgets
  leaveObjectCriticalSection();

  if (propagate) BROBase_kill(propagate);
};


// Destructor
VROWidget::~VROWidget()
{
  VROWidget_kill(false);
};


// Calls the static version of the function called by rexx
int VROWidget::functionHandler(std::string name, std::vector<std::string> &args, std::string &ret)
{
  LOGF(std::string n; getName(n),"VROWidget(" + n + ")::functionHandler(" + name + ", args, ret)");
  int rc;

  // VROWIDGET_FUNCTION func = VROWIDGET_FUNCTIONs[name];
  VROWIDGET_FUNCTION func = 0;
  std::map<std::string,VROWIDGET_FUNCTION>::iterator iter = VROWIDGET_FUNCTIONs.find(name);
  if ( iter!=VROWIDGET_FUNCTIONs.end() ) func = iter->second;

  if (!func) return BROBase::functionHandler(name, args, ret); // Send request to superclass

  rc = func(this, args, ret); // 0 = success. Positive Number = function specific error
  if (rc<0) rc = -rc; // Make sure it is postive - i.e. not to be confused with function not found
  return rc;
}


// Calls the static version of the function called by rexx
int VROWidget::procedureHandler(std::string name, std::vector<std::string> &args)
{
  LOGF(std::string n; getName(n),"VROWidget(" + n + ")::procedureHandler(" + name + ", args)");
  int rc;
  std::string ret;

//  VROWIDGET_FUNCTION func = VROWIDGET_FUNCTIONs[name];
  VROWIDGET_FUNCTION func = 0;
  std::map<std::string,VROWIDGET_FUNCTION>::iterator iter = VROWIDGET_FUNCTIONs.find(name);
  if ( iter!=VROWIDGET_FUNCTIONs.end() ) func = iter->second;

  if (!func) return BROBase::procedureHandler(name, args); // Send request to superclass

  rc = func(this, args, ret); // 0 = success. Positive Number = function specific error
  if (rc<0) rc = -rc; // Make sure it is postive - i.e. not to be confused with function not found
  return rc;
}

// Static callback handler for fltk gui callbacks
void VROWidget::callbackHandler(fltk::Widget* w, void* data)
{
  LOG("VROWidget::callbackHandler()");
  VROWidget* context = (VROWidget*)data;

  IFLOG(isChildThread(),"VROWidget::callbackHandler() - Child Thread!");

  LOGF(std::string n; context->getName(n), "VROWidget::callbackHandler() ... context name: " + n);

  /* The following is needed for OOREXX
     Somehow OOREXX allows Windows Events while it it processing
     so this code ignores callback events
     if rexx is already running in this thread */
  if (rexxRunning())
  {
    LOG("VROWidget::callbackHandler() ... REXX already running in this thread! - returning");
    return;
  }
  //context->deactivate();
  context->executeRexx("CALLBACKFUNCTION");
  //context->activate();
}


int VROWidget::staticSetExternalCallback(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  if (args.size()!=1) {
    ret="1";
  }
  else {
    object->setExternalCallback(args[0]);
    ret="0";
  }
  return 0;
};


// Function to set the widget callback
void VROWidget::setExternalCallback(std::string rexxfunction)
{
  enterObjectCriticalSection();
  setExternalRexx("CALLBACKFUNCTION", rexxfunction);

  if (widget)
  {
    if (isChildThread()) fltk::lock();
    if (rexxfunction.size()) widget->callback(VROWidget::callbackHandler, this);
    else widget->callback(0, (BROBase*)0);
    if (isChildThread()) fltk::unlock();
  }
  leaveObjectCriticalSection();
}

int VROWidget::staticSetCallback(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  if (args.size()!=1) {
    ret="1";
  }
  else {
    object->setCallback(args[0]);
    ret="0";
  }
  return 0;
};


void VROWidget::setWidget(fltk::Widget* _widget)
{
  enterObjectCriticalSection();
  widget = _widget;
  if (widget)
  {
    if (isChildThread()) fltk::lock();

    if (rexxFuncExists("CALLBACKFUNCTION")) widget->callback(VROWidget::callbackHandler, this);
    else widget->callback(0, (BROBase*)0);;

    if (isChildThread()) fltk::unlock();
  }
  leaveObjectCriticalSection();
};


fltk::Widget* VROWidget::getWidget()
{
  fltk::Widget* ret;
  enterObjectCriticalSection();
  ret=widget;
  leaveObjectCriticalSection();
  return ret;
};


// Function to set the widget callback
void VROWidget::setCallbackWithStatic(const char* rexxcode)
{
  enterObjectCriticalSection();
  setRexxWithStatic("CALLBACKFUNCTION", rexxcode);

  if (widget)
  {
    if (isChildThread()) fltk::lock();
    if (rexxcode) widget->callback(VROWidget::callbackHandler, this);
    else widget->callback(0, (BROBase*)0);
    if (isChildThread()) fltk::unlock();
  }
  leaveObjectCriticalSection();
};


// Function to set the widget callback
void VROWidget::setCallback(std::string& rexxcode)
{
  enterObjectCriticalSection();
  setRexx("CALLBACKFUNCTION", rexxcode);

  if (widget)
  {
    if (isChildThread()) fltk::lock();
    if (rexxcode.size()) widget->callback(VROWidget::callbackHandler, this);
    else widget->callback(0, (BROBase*)0);
    if (isChildThread()) fltk::unlock();
  }
  leaveObjectCriticalSection();
};


// Rexx Function to force a redraw
int VROWidget::staticFlush(VROWidget* context, std::vector<std::string> &args, std::string &ret)
{
  flush();
  return 0;
}


// Function to force a redraw
void VROWidget::flush()
{
  if (isChildThread())
  {
    fltk::lock();
    fltk::awake();
    fltk::unlock();
  }
  else fltk::flush();
};


// Rexx Function to hide the widget
int VROWidget::staticHide(VROWidget* context, std::vector<std::string> &args, std::string &ret)
{
  context->hide();
  return 0;
}


// Function to hide the widget
void VROWidget::hide()
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->hide();
    if (isChildThread()) fltk::unlock();
  }
};


// Rexx Function to show the widget
int VROWidget::staticShow(VROWidget* context, std::vector<std::string> &args, std::string &ret)
{
  context->show();
  return 0;
}


// Function to show the widget
void VROWidget::show()
{
#ifdef WIN32
  if (isChildThread())
  {
    enterStaticCriticalSection();
    needShow.push(this);
    signalMainThread = true;
    leaveStaticCriticalSection();
    fltk::lock();
    fltk::awake();
    fltk::unlock();
  }
  else
  {
    fltk::Widget* w;
    enterObjectCriticalSection();
    w=widget;
    leaveObjectCriticalSection();
    if (w) w->show();
  }
#else
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->show();
    if (isChildThread()) fltk::unlock();
  }
#endif
};


// Rexx Function to activate the widget
int VROWidget::staticActivate(VROWidget* context, std::vector<std::string> &args, std::string &ret)
{
  context->activate();
  return 0;
}


// Function to activate the widget
void VROWidget::activate()
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->activate();
    if (isChildThread()) fltk::unlock();
  }
};


// Rexx Function to deactivate the widget
int VROWidget::staticDeactivate(VROWidget* context, std::vector<std::string> &args, std::string &ret)
{
  context->deactivate();
  return 0;
}


// Function to deactivate the widget
void VROWidget::deactivate()
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->deactivate();
    if (isChildThread()) fltk::unlock();
  }
};


// Set box style from rexx
int VROWidget::staticBoxType(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  fltk::Box* t;
  if (args.size()!=1) return -1;

  if (args[0]=="UP_BOX") t=fltk::UP_BOX;
  else if (args[0]=="DOWN_BOX") t=fltk::DOWN_BOX;
  else if (args[0]=="THIN_UP_BOX") t=fltk::THIN_UP_BOX;
  else if (args[0]=="THIN_DOWN_BOX") t=fltk::THIN_DOWN_BOX;
  else if (args[0]=="ENGRAVED_BOX") t=fltk::ENGRAVED_BOX;
  else if (args[0]=="EMBOSSED_BOX") t=fltk::EMBOSSED_BOX;
  else if (args[0]=="BORDER_BOX") t=fltk::BORDER_BOX;
  else if (args[0]=="FLAT_BOX") t=fltk::FLAT_BOX;
  else if (args[0]=="HIGHLIGH_UP_BOX") t=fltk::HIGHLIGHT_UP_BOX;
  else if (args[0]=="HIGHLIGH_DOWN_BOX") t=fltk::HIGHLIGHT_DOWN_BOX;
  else if (args[0]=="ROUND_UP_BOX") t=fltk::ROUND_UP_BOX;
  else if (args[0]=="ROUND_DOWN_BOX") t=fltk::ROUND_DOWN_BOX;
  else if (args[0]=="DIAMOND_UP_BOX") t=fltk::DIAMOND_UP_BOX;
  else if (args[0]=="DIAMOND_DOWN_BOX") t=fltk::DIAMOND_DOWN_BOX;
  else if (args[0]=="NO_BOX") t=fltk::NO_BOX;
  else if (args[0]=="SHADOW_BOX") t=fltk::SHADOW_BOX;
  else if (args[0]=="ROUNDED_BOX") t=fltk::ROUNDED_BOX;
  else if (args[0]=="RSHADOW_BOX") t=fltk::RSHADOW_BOX;
  else if (args[0]=="RFLAT_BOX") t=fltk::RFLAT_BOX;
  else if (args[0]=="OVAL_BOX") t=fltk::OVAL_BOX;
  else if (args[0]=="OSHADOW_BOX") t=fltk::OSHADOW_BOX;
  else if (args[0]=="OFLAT_BOX") t=fltk::OFLAT_BOX;
  else if (args[0]=="BORDER_FRAME") t=fltk::BORDER_FRAME;
  // else if (args[0]=="DOTTED_FRAME") t=fltk::DOTTED_FRAME;
  else if (args[0]=="PLASTIC_UP_BOX") t=fltk::PLASTIC_UP_BOX;
  else if (args[0]=="PLASTIC_DOWN_BOX") t=fltk::PLASTIC_DOWN_BOX;
  else return -1;

  object->boxType(t);
  return 0;
};


// Set box style
void VROWidget::boxType(fltk::Box* btype)
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->box(btype);
    if (isChildThread()) fltk::unlock();
  }
}


// Set label font from rexx
int VROWidget::staticLabelFont(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  fltk::Font* f;
  if (args.size()!=1) return -1;

  if (args[0]=="HELVETICA") f=fltk::HELVETICA;
  else if (args[0]=="HELVETICA") f=fltk::HELVETICA_BOLD;
  else if (args[0]=="HELVETICA") f=fltk::HELVETICA_ITALIC;
  else if (args[0]=="HELVETICA") f=fltk::HELVETICA_BOLD_ITALIC;
  else if (args[0]=="COURIER") f=fltk::COURIER;
  else if (args[0]=="COURIER") f=fltk::COURIER_BOLD;
  else if (args[0]=="COURIER") f=fltk::COURIER_ITALIC;
  else if (args[0]=="COURIER") f=fltk::COURIER_BOLD_ITALIC;
  else if (args[0]=="TIMES") f=fltk::TIMES;
  else if (args[0]=="TIMES") f=fltk::TIMES_BOLD;
  else if (args[0]=="TIMES") f=fltk::TIMES_ITALIC;
  else if (args[0]=="TIMES") f=fltk::TIMES_BOLD_ITALIC;
  else if (args[0]=="SYMBOL") f=fltk::SYMBOL_FONT;
  else if (args[0]=="SCREEN") f=fltk::SCREEN_FONT;
  else if (args[0]=="SCREEN_BOLD") f=fltk::SCREEN_BOLD_FONT;
  else if (args[0]=="DINGBATS") f=fltk::ZAPF_DINGBATS;
  else return -1;

  object->labelFont(f);
  return 0;
}


// Set label font
void VROWidget::labelFont(fltk::Font* font)
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->labelfont(font);
    if (isChildThread()) fltk::unlock();
  }
}


// Set label font size from rexx
int VROWidget::staticLabelSize(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  if (args.size()!=1) return -1;
  float size = atof(args[0].c_str());
  if (!size) return -1;

  object->labelSize(size);
  return 0;
}


// Set label font size
void VROWidget::labelSize(float size)
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->labelsize(size);
    if (isChildThread()) fltk::unlock();
  }
}


// Set Label style from rexx
int VROWidget::staticLabelType(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  fltk::LabelType* l;
  if (args.size()!=1) return -1;

  if (args[0]=="NO_LABEL") l=fltk::NO_LABEL;
  else if (args[0]=="NORMAL_LABEL") l=fltk::NORMAL_LABEL;
  else if (args[0]=="SYMBOL_LABEL") l=fltk::SYMBOL_LABEL; // same as NORMAL_LABEL
  else if (args[0]=="SHADOW_LABEL") l=fltk::SHADOW_LABEL;
  else if (args[0]=="ENGRAVED_LABEL") l=fltk::ENGRAVED_LABEL;
  else if (args[0]=="EMBOSSED_LABEL") l=fltk::EMBOSSED_LABEL;
  else return -1;

  object->labelType(l);
  return 0;
}


// Set Label style
void VROWidget::labelType(fltk::LabelType* ltype)
{
  fltk::Widget* w;
  enterObjectCriticalSection();
  w=widget;
  leaveObjectCriticalSection();
  if (w)
  {
    if (isChildThread()) fltk::lock();
    w->labeltype(ltype);
    if (isChildThread()) fltk::unlock();
  }
}


// Function to show an input dialog
int VROWidget::staticGetInput(VROWidget* object, std::vector<std::string> &args, std::string &ret)
{
  if (args.size()==0) {
    getInput("Prompt", ret);
  }
  else if (args.size()==1) {
    getInput(args[0], ret);
  }
  else {
    ret="";
    return 1; // Invalid Arguments
  }
  return 0;
};


// Function to show an input dialog
void VROWidget::getInput(std::string prompt, std::string &entered)
{
  if (isChildThread())
  {
    GetInputRequest request;
    request.prompt = prompt;

#ifdef WIN32
    request.requestDone = CreateEvent(
            NULL,     // no security attributes
            FALSE,    // auto-reset event
            FALSE,    // initial state is not signaled
            NULL);    // object not named
#else
    pthread_cond_init(&(request.requestDone), NULL);
    pthread_mutex_init(&(request.requestDoneMutex), NULL);
    pthread_mutex_lock(&(request.requestDoneMutex));
#endif
    enterStaticCriticalSection();
    needGetInput.push(&request);
    signalMainThread = true;
    leaveStaticCriticalSection();
    fltk::lock();
    fltk::awake();
    fltk::unlock();
#ifdef WIN32
    WaitForSingleObject(request.requestDone, INFINITE);
    CloseHandle(request.requestDone);
#else
    pthread_cond_wait(&(request.requestDone), &(request.requestDoneMutex));
    pthread_mutex_unlock(&(request.requestDoneMutex));
    pthread_cond_destroy(&(request.requestDone));
    pthread_mutex_destroy(&(request.requestDoneMutex));
#endif
    entered = request.result;
  }
  else
  {
    const char *in = fltk::input(prompt.c_str());
    entered.clear();
    if (in) entered.append(in);
  }
};


/********************************************************************/
/* Exception Class Functions                                        */
/********************************************************************/

BROFailedToCreateGuiClass::BROFailedToCreateGuiClass(std::string message) :
                           BROException("BROFailedToCreateGuiClass: " + message)
{};

BROFailedToCreateGuiClass::BROFailedToCreateGuiClass() :
                           BROException("BROFailedToCreateGuiClass")
{};
