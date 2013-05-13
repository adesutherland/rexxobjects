// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : test/botestlib.cpp
// Description : Unit Tests
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
// *************************************************************************/

#include "object.h"

using namespace std;

int doGetRexxVersionTest1() {
  int rc, result=0;
  vector<string> args;
  string ret;
  BROBase::lineOut << "GetRexxVersion Test 1" << flush;

  BROBase* test = BROBaseFactory::newObject(0, "BROBase", "test");
  if (!test)
  {
       BROBase::errorOut << "  > Error - BROBase object not created" << flush;
       result = 1;
  }
  else
  {
       rc = test->callFunction("GetRexxVersion", args, ret);
       BROBase::lineOut << "  > RC = " << rc << flush;
       BROBase::lineOut << "  > Result = " << ret << flush;

       if (rc)
       {
           result = 1;
           BROBase::lineOut << "  > Failure RC should be 0" << flush;
       }

       if (ret.length() == 0)
       {
           result = 1;
           BROBase::lineOut << "  > Zero Length Result Returned" << flush;
       }

       delete test;
  }

  BROBase::lineOut << "  > Test Complete" << flush;

  return result;
};


int doThreadTest2() {
  int rc, result=0;
  vector<string> args;
  string ret;
  BROBase::lineOut << "Thread Test 1" << flush;

  BROBase* test = BROBaseFactory::newObject(0, "BROBase", "test");
  if (!test)
  {
       BROBase::errorOut << "  > Error - BROBase object not created" << flush;
       result = 1;
  }
  else
  {
       test->setRexx("ThreadTest","say 'Hello'");
       test->setRexx("Test","x=startThread('ThreadTest'); call sleep 1; say 'Thread called'; return x");
       rc = test->callFunction("Test", args, ret);
       BROBase::lineOut << "  > RC = " << rc << flush;
       BROBase::lineOut << "  > Result = " << ret << flush;

       if (rc)
       {
           result = 1;
           BROBase::lineOut << "  > Failure RC should be 0" << flush;
       }


       delete test;
  }

  BROBase::lineOut << "  > Test Complete" << flush;

  return result;
};






int doBOTests() {
  int ok=0, failed=0, rc;
  BROBase::lineOut << "RexxObjects BaseObjects Tests" << flush;

  rc = doGetRexxVersionTest1();
  if (rc) failed++;
  else ok++;

  rc = doThreadTest2();
  if (rc) failed++;
  else ok++;

  BROBase::lineOut << "RexxObjects BaseObjects Test Results" << flush;

  BROBase::lineOut << "Tests Run OK = " << ok << flush;
  BROBase::lineOut << "Tests Failed = " << failed << flush;
  return failed;
};

