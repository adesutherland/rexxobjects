// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : rexxfunc.h
// Description : C++ Header for Rexx API Interface code
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

#ifndef ro_bo_rexxfunc_h
#define ro_bo_rexxfunc_h

#include <string>
#include <vector>
#include <stack>

// Needed for rexx functions
#define INCL_REXXSAA
#ifdef _REXX
#include <rexx.h>
#endif
#ifdef _REXXSAA
#include <rexxsaa.h>
#define RexxWaitForTermination()
#endif
#ifdef _REXXTRANS
#include <rexxtrans.h>
#endif

/* This macro is probably not needed - but the oorexx manual does state that
   nulls are not added in all situations (although they do seem to be).
   The problem with this macro is that we could very well buffer overflow,
   so you need to be SURE that the buffer is big enough */
#define RXADDNULL(r) {if ((r).strptr) (r).strptr[(r).strlength]=0;}

enum rexx_type {NOTSET, UNKNOWN, REGINA, OREXX};

// Class to hold variables
class BROVar
{
  public:
   std::string name;
   std::string value;
   BROVar(std::string &nm) {name=nm;};
};

// Class to hold Stems
class BROStem
{
  public:
   std::string name;
   std::vector<std::string> values;
   BROStem(std::string &nm) {name=nm;};
};

// Class to hold Filtered rexx variables
class BROFilter
{
  public:
   std::string filter;
   std::vector<std::string> names;
   std::vector<std::string> values;
   BROFilter(std::string &fl) {filter=fl;};
};

class BROBase;

// Class to hold a rexx function
class BRORexxFunction
{
  friend class BROBase;

  private:
    static RXSYSEXIT exitlist[3];
    static void* rexxContextIndex;
    static std::stack<BRORexxFunction*> mainThreadContext;
    static bool doneInit;
    static rexx_type rexxType;
    static std::string rexxVersion;

    std::string name;
    BROBase* parent;
    std::string externalrexx;
    void* rexx;
    std::string* rexxcode;
    static LONG APIENTRY RexxIOExit(LONG ExitNumber, LONG Subfunction, PEXIT parmblock);
    static LONG APIENTRY RexxFncExit(LONG ExitNumber, LONG Subfunction, PEXIT parmblock);
    static void popRexxContext();
    static void pushRexxContext(BRORexxFunction* context);
    static BRORexxFunction* peekRexxContext();
    static int GetRexxVersion(std::string &version);

  public:
    BRORexxFunction(BROBase* pnt, std::string _name);
    ~BRORexxFunction();

    static int InitRexx();
    static int DoneWithRexx();
    static int SetRexxVariable(std::string name, std::string& value);
    static int SetRexxVariable(std::string name, std::vector<std::string>& values);
    static int SetRexxVariable(std::vector<std::string>& names, std::vector<std::string>& values);
    static int SetRexxVariable(BROVar* globals);
    static int SetRexxVariable(BROStem* globals);
    static int SetRexxVariable(BROFilter* globals);
    static int GetRexxVariable(std::string name, std::string &value );
    static int GetRexxVariable(std::string name, std::vector<std::string> &values );
    static int GetRexxVariable(BROVar* globals);
    static int GetRexxVariable(BROStem* globals);
    static int GetRexxVariable(BROFilter* globals);
    static bool isValidFilter(std::string& word);
    static bool isValidStem(std::string& word);
    static bool isValidVar(std::string& word);

    int executeRexx();
    int executeRexx(std::vector<std::string>& args);
    int executeRexx(std::vector<std::string>& args, std::string& ret);
    void setRexx(std::string& rexxcode);
    void setRexxWithStatic(const char* rexxcode);
    void setExternalRexx(std::string& rexxfunctionName);
};

#endif
