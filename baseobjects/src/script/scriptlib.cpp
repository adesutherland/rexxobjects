// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2009 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : script/scriptlib.cpp
// Description : Library wrapper to allow a Rexx program to be run as a
// script (in boot loader mode - object BOOT, Function LOADER). This has
// access to the base objects shell and has the possibility of adding functions
// and objects and global variables etc.
// If the REXX function returns a object name that exists
// with a function called MAIN then the BOOT Object is deleted and then
// application.main() is executed. This is designed to allow a Bootstrap REXX
// program to set-up an environment for an application to run.
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

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>

#include "object.h"

#ifdef WIN32
char *realpath(const char *path, char resolved_path[PATH_MAX]);
#endif


using namespace std;

// For string to int (and other nums)
template <class T>
bool from_string(T& t, const std::string& s,
                 std::ios_base& (*f)(std::ios_base&))
{
  std::istringstream iss(s);
  return !(iss >> f >> t).fail();
}

int bootstrapMain(string exeName, const char* bootstrap, vector<string> &args)
{
  string ret = "";
  int rc = 0;

  // Get the full/actual exe name and path
  char actualpath[PATH_MAX];
  if (!realpath(exeName.c_str(), actualpath))
  {
      cerr << "WARNING. File not found unexpectedly when trying to get actual path: " << exeName << endl;
  }
  else exeName=actualpath;

  // Set-up
  BROBase::InitObject(exeName);

  BROBase* mainObject = BROBaseFactory::newObject(0, "BROBase", "BOOT");
  if (!mainObject) cerr << "Error: mainObject not created" << endl;

  mainObject->setRexxWithStatic("LOADER", bootstrap);
  rc = mainObject->executeRexx("LOADER", args, ret);

  // If the REXX function returns a object name that exists
  // with a function called MAIN then the BOOT Object is deleted and then
  // application.main() is executed.
  if (!rc && ret.length())
  {
     BROBase* app = mainObject->findRoot(ret);
     if (app)
     {
       if (app->rexxFuncExists("MAIN"))
       {
         delete mainObject;
         rc = app->executeRexx("MAIN", args, ret);
       }
     }
  }
  // Clean-up
  BROBase::DoneWithObject();

  if (rc)
  {
      cerr << "Error Executing Rexx, RC=" << rc << endl;
      return -1;
  }
  else if(from_string<int>(rc, ret, dec))
  {
    return rc;
  }
/* Don't need this - after all the REXX program can just "say" any output as needed by the application
  else if (ret.length()>0)
  {
    cout << "Return=" << ret << endl;
  }
*/
  return 0;
};

