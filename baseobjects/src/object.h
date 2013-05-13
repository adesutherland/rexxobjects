// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : object.h
// Description : C++ Header of main/base object
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

#ifndef ro_bo_object_h
#define ro_bo_object_h

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "exception.h"
#include "thread.h"

// Function to allow a Rexx program to be run as a
// script (in boot loader mode - object BOOT, Function LOADER). This has
// access to the base objects shell and has the possibility of adding functions
// and objects and global variables etc.
// If the REXX function returns a object name that exists
// with a function called MAIN then the BOOT Object is deleted and then
// application.main() is executed. This is designed to allow a Bootstrap REXX
// program to set-up an environment for an application to run.
int bootstrapMain(const char* bootstrap, std::vector<std::string> &args);

// Functions for external functions (to be called from rexx)
class BROBase;
typedef int(*BROBase_FUNCTION)(BROBase* object, std::vector<std::string> &args, std::string &ret);

// Thread "main" Functions - void function(BROBase* object, void* param)
typedef void(*THREAD_FUNCTION)(BROBase* object, void* param);

class BROBaseFactory;

// rexxapi.h classes
class BRORexxFunction;
class BROVar;
class BROStem;
class BROFilter;

// Console Classes
typedef void(*BROLINEOUT_FUNCTION)(std::string text);

class BROStreamBuffer : public std::stringbuf
{
public:
    BROStreamBuffer(BROLINEOUT_FUNCTION func) : std::stringbuf(std::ios::out) {OutFunc=func;}
protected:
    virtual int sync();
private:
    BROLINEOUT_FUNCTION OutFunc;
};

class BROStream : public std::ostream
{
public:
    BROStream(BROLINEOUT_FUNCTION func) : std::ostream(new BROStreamBuffer(func)) {}
    ~BROStream()
    {
        delete rdbuf();
    }
};

class BROConsoleIO
{
 friend class BROBase;

 public:
  BROConsoleIO() {};
  virtual ~BROConsoleIO() {};
  // Console Routines
 protected:
  virtual void LineOut(std::string text);
  virtual void ErrorOut(std::string text);
  virtual void TraceOut(std::string text);
  virtual void GetInput(std::string &entered);
};

class BROBase
{
 friend class BROBaseFactory;

 private:
  static bool ObjectInitted;
  static std::map<std::string, BROBase_FUNCTION> BROBase_functions;
  static std::map<std::string, BROBase*> roots;
  static unsigned long mainThreadId;
  static void* threadIndex;
  static std::string exeName;

  std::map<std::string, BRORexxFunction*> RexxFunctions;
  std::string name;
  BROBase* parent;
  std::map<std::string, BROBase*> children;
  void BROBase_setup(BROBase* parent, std::string name);
  std::map<std::string, BROVar*> globalVars;
  std::map<std::string, BROStem*> globalStems;
  std::map<std::string, BROFilter*> globalFilters;
  void* objectMutex;       // Pointer to object thread lock
  static void* staticMutex;// Pointer to overall (static) thread lock
  bool zombie;             // Set to true if the object is being deleted

 protected:
  const char* type;
  static BROConsoleIO* consoleIO;
  void BROBase_kill(bool propagate);
  BROBase(BROBase* parent, std::string name);
  BROBase(BROBase* parent, std::string name, std::vector<std::string> &args);
  virtual void beginNewObject();
  virtual void endNewObject();
  static void getFirstPart(std::string& path, std::string& first, std::string& rest);
  static void getLastPart(std::string& path, std::string& last, std::string& rest);
  static void getLastStem(std::string& path, std::string& stem, std::string& rest);
  static bool rexxRunning();

  // Critical Section (Mutex)
  // The rule is that static mutex should be held first, then parent object,
  // then current object, then children objects.
  // Only one child object or sibling allowed at a time
  int enterObjectCriticalSectionIfNotZombie();
  void enterObjectCriticalSection();
  void leaveObjectCriticalSection();
  static void enterStaticCriticalSection();
  static void leaveStaticCriticalSection();
  int startThread(THREAD_FUNCTION function, void* param);
  static void setError(int rc, std::string text);

 public:
  virtual ~BROBase();
  static void InitObject();
  static void InitObject(std::string exeName);
  static void DoneWithObject();

  // Console Routines
  static BROStream lineOut;
  static BROStream errorOut;
  static BROStream traceOut;
  static void LineOut(std::string text);
  static void ErrorOut(std::string text);
  static void TraceOut(std::string text);
  static void GetInput(std::string &entered);

  // Misc Routines
  static void upperCase(std::string &word);

  // Thread routines
  static bool isChildThread();
  static bool isMainThread();
  static void enterCriticalSection(void* mutex);
  static void leaveCriticalSection(void* mutex);
  static void createCriticalSection(void* &mutex);
  static void deleteCriticalSection(void* &mutex);
  static int staticStartThread(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static void ThreadMain(BROBase* object, void *param);
  static ThreadObject* getThreadObject();

  // Functions to call the rexx interpreter
  int executeRexx(std::string function);
  int executeRexx(std::string function, std::vector<std::string>& args);
  int executeRexx(std::string function, std::vector<std::string>& args, std::string& ret);
  static int staticSetExternalRexx(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticSetRexx(BROBase* object, std::vector<std::string> &args, std::string &ret);
  void setRexx(std::string function, std::string rexxfunction);
  void setRexxWithStatic(std::string function, const char* rexxfunction);
  void setExternalRexx(std::string function, std::string rexxfunction);
  bool rexxFuncExists(std::string function);

  // Functions called by rexx handlers
  virtual int functionHandler(std::string name, std::vector<std::string> &args, std::string &ret);
  static int staticCallFunction(BROBase* object, std::vector<std::string> &args, std::string &ret);
  int callFunction(std::string target, std::string name, std::vector<std::string> &args, std::string &ret);
  int callFunction(std::string target_and_name, std::vector<std::string> &args, std::string &ret);
  virtual int procedureHandler(std::string name, std::vector<std::string> &args);
  static int staticCallProcedure(BROBase* object, std::vector<std::string> &args, std::string &ret);
  int callProcedure(std::string target, std::string name, std::vector<std::string> &args);
  int callProcedure(std::string target_and_name, std::vector<std::string> &args);

  // Class functions - also callable from Rexx
  static int staticNewObject(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROBase* newObject(std::string objectType, std::string objectName, std::vector<std::string> &args);
  BROBase* newObject(std::string objectType, std::string objectName);
  static int staticNewRootObject(BROBase* object, std::vector<std::string> &args, std::string &ret);

  static int staticGetParent(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROBase* getParent();

  static int staticFindChild(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROBase* findChild(std::string name);

  static int staticListChildren(BROBase* object, std::vector<std::string> &args, std::string &ret);
  int listChildren(std::vector<BROBase*> &children);

  static int staticFindRoot(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static BROBase* findRoot(std::string name);

  static int staticFindObject(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROBase* findObject(std::string& name);

  static int staticDeleteObject(BROBase* object, std::vector<std::string> &args, std::string &ret);

  static int staticListRoot(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int listRoot(std::vector<BROBase*> &roots);

  static int staticGetType(BROBase* object, std::vector<std::string> &args, std::string &ret);
  void getType(std::string &type);

  static int staticGetName(BROBase* object, std::vector<std::string> &args, std::string &ret);
  void getName(std::string &Name);

  static int staticGetExeName(BROBase* object, std::vector<std::string> &args, std::string &ret);
  void getExeName(std::string &exeName);

  static int staticGetFullName(BROBase* object, std::vector<std::string> &args, std::string &ret);
  void getFullName(std::string &Name);

  static int staticAddGlobalVar(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROVar* addGlobalVar(std::string& name);
  BROVar* addGlobalVar(char* _name);

  static int staticAddGlobalStem(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROStem* addGlobalStem(std::string& name);
  BROStem* addGlobalStem(char* _name);

  static int staticAddGlobalFilter(BROBase* object, std::vector<std::string> &args, std::string &ret);
  BROFilter* addGlobalFilter(std::string& name);
  BROFilter* addGlobalFilter(char* _name);

  static int staticIsValidVar(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticIsValidStem(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticIsValidFilter(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticLoadVar(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticSaveVar(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticLoadStem(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticSaveStem(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticLoadFilter(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticSaveFilter(BROBase* object, std::vector<std::string> &args, std::string &ret);

  static int staticIsChildThread(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticIsMainThread(BROBase* object, std::vector<std::string> &args, std::string &ret);
  static int staticRexxFuncExists(BROBase* object, std::vector<std::string> &args, std::string &ret);

};

// Factory Class for making base objects
class BROBaseFactory
{
 private:
  static std::map<std::string, BROBaseFactory*>* factories;
  std::string objectType;
  virtual BROBase* newObject(BROBase* parent, std::string name);
  virtual BROBase* newObject(BROBase* parent, std::string name, std::vector<std::string> &args);

 protected:
  BROBaseFactory(const char* objectType);

 public:
  BROBaseFactory();
  virtual ~BROBaseFactory();
  static  BROBase* newObject(BROBase* parent, std::string objectType, std::string objectName);
  static  BROBase* newObject(BROBase* parent, std::string objectType, std::string objectName,
                               std::vector<std::string> &args);
  static  BROBaseFactory* findFactory(std::string objectType);
};


// Exceptions
class BRODuplicateChildName : public BROException
{
 public:
  BRODuplicateChildName(std::string message);
  BRODuplicateChildName();
};

class BRODuplicateRootName : public BROException
{
 public:
  BRODuplicateRootName(std::string message);
  BRODuplicateRootName();
};

class BROUnknownObjectType : public BROException
{
 public:
  BROUnknownObjectType(std::string message);
  BROUnknownObjectType();
};

class BROInvalidConstructorArguments : public BROException
{
 public:
  BROInvalidConstructorArguments(std::string message);
  BROInvalidConstructorArguments();
};

class BROCriticalSectionError : public BROException
{
 public:
  BROCriticalSectionError(std::string message);
  BROCriticalSectionError();
};

class BROBaseNoLongerExistsError : public BROException
{
 public:
  BROBaseNoLongerExistsError(std::string message);
  BROBaseNoLongerExistsError();
};


#endif
