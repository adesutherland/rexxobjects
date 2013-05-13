// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : rexxfunc.cpp
// Description : Rexx API Interface code
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

#include <stdio.h>                     /* needed for printf()        */
#include <string.h>                    /* needed for strlen()        */
#include <stdlib.h>

#include "debug.h"
#include "rexxfunc.h"
#include "shell.h"    // for the REXXOBJECTS Shell
#include "object.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

/********************************************************************/
/* BRORexxFunction Functions                                        */
/********************************************************************/

using namespace std;

RXSYSEXIT BRORexxFunction::exitlist[3];              /* REXX Exit list array            */

// Points to the current object running a rexx function (main thread)
std::stack<BRORexxFunction*> BRORexxFunction::mainThreadContext;

// A thread variable holding a stack with the current object running a rexx function at the head
void* BRORexxFunction::rexxContextIndex = 0;

bool BRORexxFunction::doneInit = false;               // Has rexx been set-up?

rexx_type BRORexxFunction::rexxType = NOTSET;

string BRORexxFunction::rexxVersion;

int BRORexxFunction::InitRexx() {
  LOG("BRORexxFunction::InitRexx()");

  if (doneInit) return -1;
  doneInit = true;

  #ifdef WIN32
   rexxContextIndex = (void*)(new DWORD);
   if ((*(DWORD*)rexxContextIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES) return -1;
  #else
   rexxContextIndex = (void*)(new pthread_key_t);
   pthread_key_create((pthread_key_t*)rexxContextIndex, NULL);
  #endif

  // Rexx SHELL handler
  RexxRegisterSubcomExe("RXOSHELL",    /* register shell handler */
                        (PFN)&RXO_SHELL,  /* located at this address */
                        (PUCHAR)0);         /* user info */

  // Rexx EXIT handlers
  RexxRegisterExitExe("RexxIOExit",
        (PFN)&BRORexxFunction::RexxIOExit,
        NULL);
  RexxRegisterExitExe("RexxFncExit",
        (PFN)&BRORexxFunction::RexxFncExit,
        NULL);

  exitlist[0].sysexit_name = "RexxIOExit";
  exitlist[0].sysexit_code = RXSIO;
  exitlist[1].sysexit_name = "RexxFncExit";
  exitlist[1].sysexit_code = RXFNC;
  exitlist[2].sysexit_code = RXENDLST;

  if (GetRexxVersion(rexxVersion)) return -1;
  string ver = rexxVersion.substr(0,11);
  if (ver == "REXX-Regina") rexxType=REGINA;
  else if (ver == "REXX-ooRexx") rexxType=OREXX;
  else
  {
      rexxType=UNKNOWN;
      cout << "WARNING REXX Version Unknown: " << rexxVersion << endl;
  }
  return 0;
};


int BRORexxFunction::GetRexxVersion(string &version)
{
    LOG("BRORexxFunction::GetRexxVersion(name,value)");
/*  Version 1 - Try getting the special version variable - failed */
//    SHVBLOCK block; /* variable pool control block*/
//    block.shvcode = RXSHV_PRIV; /* do a get operation*/
//    block.shvret=(UCHAR)0; /* clear return code field */
//    block.shvnext=(PSHVBLOCK)0; /* no next block */
//    block.shvvalue.strptr = NULL;
//    block.shvvalue.strlength = 0;
//    MAKERXSTRING(block.shvname, (char*)"VERSION", 7);
//
//    int rc = RexxVariablePool(&block); /* get the version */
//
//    if (block.shvvalue.strptr)
//    {
//      version.assign(block.shvvalue.strptr, block.shvvalue.strlength);
//      RexxFreeMemory(block.shvvalue.strptr);
//    }
//    else version.clear();

    /* Version 2 - execute a rexx function to do it {sigh} */
    char* func = "PARSE VERSION V\nRETURN V\n";

    APIRET   rc;                        /* return code from REXX  */
    RXSTRING rexxretval;                /* return value from REXX     */

    RXSTRING rexx[2];                      // New Rexx function structure
    MAKERXSTRING(rexx[0], func, strlen(func));
    MAKERXSTRING(rexx[1], 0, 0);

    /* By setting the strlength of the output RXSTRING to zero, we    */
    /* force the interpreter to allocate memory and return it to us.  */
    rexxretval.strlength = 0L;
    rexxretval.strptr = NULL; // For OOREXX

    rc=RexxStart((LONG)     0,                    /* number of arguments    */
                (PRXSTRING) NULL,                 /* array of arguments     */
                (PSZ)       "GETVERSION.REX",     /* name of REXX file      */
                (PRXSTRING )rexx,                 /* INSTORE used           */
                (PSZ)       "RXOSHELL",           /* Command env. name      */
                (LONG)      RXFUNCTION,           /* Code for how invoked   */
                (PRXSYSEXIT)exitlist,             /* EXITs on this call     */
                (PSHORT)    0,                    /* Rexx program output    */
                (PRXSTRING) &rexxretval );        /* Rexx program output    */

    if (((PRXSTRING)rexx)[1].strptr) RexxFreeMemory(((PRXSTRING)rexx)[1].strptr);

    if (!peekRexxContext())
    {
      LOG("BRORexxFunction::GetRexxVersion() ... Calling RexxWaitForTermination()");
      RexxWaitForTermination();
    }

    // Handle Return Value
    if (rexxretval.strptr)
    {
      version.assign(rexxretval.strptr,rexxretval.strlength);
      RexxFreeMemory(rexxretval.strptr);      /* Release storage given to us by REXX */
    }
    else
    {
      version.clear();
    }

    LOG("BRORexxFunction::GetRexxVersion(): Version=" + version);
    return rc;
}


int BRORexxFunction::DoneWithRexx() {
  LOG("BRORexxFunction::DoneWithRexx()");
  if (!doneInit) return -1;

  /* wait for RexxTerminate to be posted and closed */
  LOG("BRORexxFunction::DoneWithRexx() ... Calling RexxWaitForTermination()");
  RexxWaitForTermination();

  RexxDeregisterExit("RexxIOExit",NULL);
  RexxDeregisterExit("RexxFncExit",NULL);
  RexxDeregisterSubcom("REXXOBJECTS",NULL);

  #ifdef WIN32
   TlsFree(*(DWORD*)rexxContextIndex);
   delete (DWORD*)rexxContextIndex;
  #else
   pthread_key_delete(*(pthread_key_t*)rexxContextIndex);
   delete (pthread_key_t*)rexxContextIndex;
  #endif

  rexxVersion.clear();
  rexxType = NOTSET;

  LOG("BRORexxFunction::DoneWithRexx() ... Done");
  return 0;
};


// pops off the object which was running a rexx function
void BRORexxFunction::popRexxContext()
{
  if (BROBase::isMainThread())
  {
    if (!mainThreadContext.empty()) mainThreadContext.pop();
  }
  else
  {
    std::stack<BRORexxFunction*>* stack = 0;
    #ifdef WIN32
     stack = (std::stack<BRORexxFunction*>*)TlsGetValue(*(DWORD*)rexxContextIndex);
    #else
     stack = (std::stack<BRORexxFunction*>*)pthread_getspecific(*(pthread_key_t*)rexxContextIndex);
    #endif
    if (stack)
    {
      if (!stack->empty()) stack->pop();
      if (stack->empty())
      {
        delete stack;
        #ifdef WIN32
         TlsSetValue(*(DWORD*)rexxContextIndex, 0);
        #else
         pthread_setspecific(*(pthread_key_t*)rexxContextIndex, 0);
        #endif
        if (rexxType==REGINA)
        {
          RexxDeregisterExit("RexxIOExit",NULL);
          RexxDeregisterExit("RexxFncExit",NULL);
          RexxDeregisterSubcom("REXXOBJECTS",NULL);
        }
      }
    }
  }
};


// returns the object currently running a rexx function
BRORexxFunction* BRORexxFunction::peekRexxContext()
{
  if (BROBase::isMainThread())
  {
    LOG("BRORexxFunction::peekRexxContext() - Main Thread");
    if (mainThreadContext.empty()) return 0;
    return mainThreadContext.top();
  }
  else
  {
    LOG("BRORexxFunction::peekRexxContext() - Child Thread");
    std::stack<BRORexxFunction*>* stack = 0;
    #ifdef WIN32
     stack = (std::stack<BRORexxFunction*>*)TlsGetValue(*(DWORD*)rexxContextIndex);
    #else
     stack = (std::stack<BRORexxFunction*>*)pthread_getspecific(*(pthread_key_t*)rexxContextIndex);
    #endif
    if (!stack) return 0;
    if (stack->empty()) return 0;
    return stack->top();
  }
};


// Pushes the current object running rexx
void BRORexxFunction::pushRexxContext(BRORexxFunction* context)
{
  if (BROBase::isMainThread())
  {
    mainThreadContext.push(context);
  }
  else
  {
    std::stack<BRORexxFunction*>* stack = 0;
    #ifdef WIN32
     stack = (std::stack<BRORexxFunction*>*)TlsGetValue(*(DWORD*)rexxContextIndex);
    #else
     stack = (std::stack<BRORexxFunction*>*)pthread_getspecific(*(pthread_key_t*)rexxContextIndex);
    #endif
    if (!stack)
    {
      stack = new std::stack<BRORexxFunction*>;
      #ifdef WIN32
       TlsSetValue(*(DWORD*)rexxContextIndex, (void*)stack);
      #else
       pthread_setspecific(*(pthread_key_t*)rexxContextIndex, (void*)stack);
      #endif
      if (rexxType==REGINA)
      {
        // Rexx SHELL handler
        RexxRegisterSubcomExe("RXOSHELL",    /* register shell handler */
                          (PFN)&RXO_SHELL,  /* located at this address */
                          (PUCHAR)0);         /* user info */
        // Rexx EXIT handlers
        RexxRegisterExitExe("RexxIOExit",
          (PFN)&BRORexxFunction::RexxIOExit,
          NULL);
        RexxRegisterExitExe("RexxFncExit",
          (PFN)&BRORexxFunction::RexxFncExit,
          NULL);
      }
    }
    stack->push(context);
  }
};


/*********************************************************************
 *  Description:   This is our REXX Standard input and output handler
 *********************************************************************/
LONG APIENTRY BRORexxFunction::RexxIOExit(
     LONG ExitNumber,                  /* code defining exit function*/
     LONG Subfunction,                 /* code defining exit subfunc */
     PEXIT parmblock)                  /* func dependent control bloc*/
{
   LOG("BRORexxFunction::RexxIOExit()");
   IFLOG(BROBase::isChildThread(),"BRORexxFunction::RexxIOExit() - Child Thread!");

   RXSIOSAY_PARM *sparm ;
   RXSIOTRC_PARM *tparm ;
   RXSIOTRD_PARM *pparm ;
   RXSIODTR_PARM *dparm;
   std::string in;

   switch (Subfunction) {

   case RXSIOSAY:                      /* write line to standard     */
                                       /* output stream for SAY instr*/
      sparm = ( RXSIOSAY_PARM * )parmblock ;
      BROBase::LineOut(sparm->rxsio_string.strptr);
      break;

   case RXSIOTRC:                      /* write line to standard     */
                                       /* error stream for trace or  */
                                       /* error messages             */
      tparm = ( RXSIOTRC_PARM * )parmblock ;
      BROBase::ErrorOut(tparm->rxsio_string.strptr);
      break;

   case RXSIOTRD:                      /* read line from standard    */
                                       /* input stream (PULL)        */
      pparm = ( RXSIOTRD_PARM * )parmblock ;

      BROBase::GetInput(in);
      strncpy(pparm->rxsiotrd_retc.strptr, in.c_str(), 250);
      pparm->rxsiotrd_retc.strptr[250]=0;
      pparm->rxsiotrd_retc.strlength = strlen(pparm->rxsiotrd_retc.strptr);
      break;

   case RXSIODTR:                      /* read line from standard    */
      dparm = ( RXSIODTR_PARM * )parmblock ;

      BROBase::GetInput(in);
      strncpy(dparm->rxsiodtr_retc.strptr, in.c_str(), 250);
      dparm->rxsiodtr_retc.strptr[250]=0;
      dparm->rxsiodtr_retc.strlength = strlen(dparm->rxsiodtr_retc.strptr);
      break;

   default:                            /* input stream for interactive debug */
      /* TODO */
      return RXEXIT_NOT_HANDLED;
      break;
   } /* endswitch */

   return RXEXIT_HANDLED;              /* successfully handled       */
}


/*********************************************************************
 *  Description:   This is our REXX Exit handler for function calls
 *********************************************************************/
LONG APIENTRY BRORexxFunction::RexxFncExit(
     LONG ExitNumber,                  /* code defining exit function*/
     LONG Subfunction,                 /* code defining exit subfunc */
     PEXIT parmblock)                  /* func dependent control bloc*/
{
   LOG("BRORexxFunction::RexxFncExit()");
   IFLOG(BROBase::isChildThread(),"BRORexxFunction::RexxFncExit() - Child Thread!");

   RXFNCCAL_PARM *fparm;

   switch (Subfunction) {
     case RXFNCCAL:                      /* External Function     */
       {
         fparm = (RXFNCCAL_PARM*)parmblock ;
          // typedef struct {
          //  struct {
          //   unsigned rxfferr : 1;  /* Invalid call to routine. */
          //   unsigned rxffnfnd : 1; /* Function not found. */
          //   unsigned rxffsub : 1;  /* Called as a subroutine if */
                                      /* TRUE. Return values are */
                                      /* optional for subroutines, */
                                      /* required for functions. */
          //  } rxfnc_flags ;
          //  PUCHAR rxfnc_name;      /* Pointer to function name. */
          //  USHORT rxfnc_namel;     /* Length of function name. */
          //  PUCHAR rxfnc_que;       /* Current queue name. */
          //  USHORT rxfnc_quel;      /* Length of queue name. */
          //  USHORT rxfnc_argc;      /* Number of args in list. */
          //  PRXSTRING rxfnc_argv;   /* Pointer to argument list. */
                                      /* List mimics argv list for */
                                      /* function calls, an array of */
                                      /* RXSTRINGs. */
          //  RXSTRING rxfnc_retc;    /* Return value. */
          //  } RXFNCCAL_PARM;

         BRORexxFunction* function = peekRexxContext();
         if (!function)
         {
             LOG("BRORexxFunction::RexxFncExit peekRexxContext returned null!");
             return RXEXIT_NOT_HANDLED;       /* Should never happen ... */
         }

         BROBase* context = function->parent;
         if (!context)
         {
             LOG("BRORexxFunction::RexxFncExit function->parent is null!");
             return RXEXIT_NOT_HANDLED;       /* Should never happen ... */
         }

         string functionName((char*)(fparm->rxfnc_name), (int)(fparm->rxfnc_namel));
         string ret;
         vector<string> args;

         // Set-up the arguments
         for (int i=0; i<fparm->rxfnc_argc; i++)
         {
           std::string arg((fparm->rxfnc_argv[i]).strptr, (fparm->rxfnc_argv[i]).strlength);
           args.push_back(arg);
         }
         int rc;
         if (fparm->rxfnc_flags.rxffsub)
         {
           LOG("BRORexxFunction::RexxFncExit() ... procedure " + functionName + " being called");
           rc = context->callProcedure(functionName, args);
         }
         else
         {
           LOG("BRORexxFunction::RexxFncExit() ... function " + functionName + " being called");
           rc = context->callFunction(functionName, args, ret);
         }
         if (rc<0)
         {
           LOG("BRORexxFunction::RexxFncExit() ... " + functionName + ": Function not found");
           return RXEXIT_NOT_HANDLED;
         }
         else if (rc>0)
         {
           LOG("BRORexxFunction::RexxFncExit() ... " + functionName + ": Function ERROR");
           // Function Error
           fparm->rxfnc_flags.rxfferr = 1;
           return RXEXIT_HANDLED;
         }
         LOG("BRORexxFunction::RexxFncExit() ... " + functionName + ": Function handled");
         fparm->rxfnc_flags.rxffnfnd  = 0;  // Function handled
         if (!(fparm->rxfnc_flags.rxffsub)) // Called as a functions so we have
         {                                  // to return the return value.
           fparm->rxfnc_retc.strlength = ret.size();
           if (fparm->rxfnc_retc.strlength > 255)
           {
             LOG("BRORexxFunction::RexxFncExit() ... Calling RexxAllocateMemory() for return value");
             fparm->rxfnc_retc.strptr=(char*)RexxAllocateMemory(fparm->rxfnc_retc.strlength+1);
           }
           if (ret.size()) memcpy(fparm->rxfnc_retc.strptr, ret.c_str(), fparm->rxfnc_retc.strlength+1);
           else fparm->rxfnc_retc.strptr[0]=0;
         }
       }
       break;

     default:
       return RXEXIT_NOT_HANDLED;       /* Should never happen ... */
   }

   return RXEXIT_HANDLED;              /* successfully handled       */
}

// Constructor
BRORexxFunction::BRORexxFunction(BROBase* pnt, std::string _name)
{
  if (!doneInit) InitRexx();
  rexx = 0;
  rexxcode = 0;
  parent = pnt;
  name = _name;
};

BRORexxFunction::~BRORexxFunction()
{
  // Remove any rexx function cache/memory
  setRexxWithStatic(0);
};


// Call rexx with no arguments or return value - i.e. as a simple procedure
int BRORexxFunction::executeRexx()
{
   LOG("BRORexxFunction::executeRexx()");
   IFLOG(!exitlist, "BRORexxFunction::executeRexx() ... exitlist is null");
   IFLOG(!rexx, "BRORexxFunction::executeRexx() ... rexx is null");
   IFLOG(externalrexx.size()==0, "BRORexxFunction::executeRexx() ... externalrexx zero length");
   IFLOG( rexx && ((PRXSTRING)rexx)[1].strptr , "BRORexxFunction::executeRexx() ... rexx token string exists");

   APIRET   rc;                                  /* return code from REXX  */
   PSZ rexxname;
   if (externalrexx.size()==0) rexxname = (PSZ)name.c_str();
   else rexxname = (PSZ)externalrexx.c_str();

   pushRexxContext(this);

   LOG("BRORexxFunction::executeRexx() ... Calling RexxStart");
   rc=RexxStart((LONG)     0,                    /* number of arguments    */
               (PRXSTRING) 0,                    /* array of arguments     */
               (PSZ)       rexxname,             /* name of REXX file      */
               (PRXSTRING )rexx,                 /* INSTORE used           */
               (PSZ)       "RXOSHELL",            /* Command env. name      */
               (LONG)      RXSUBROUTINE,         /* Code for how invoked   */
               (PRXSYSEXIT)exitlist,             /* EXITs on this call     */
               (PSHORT)    0,                    /* Rexx program output    */
               (PRXSTRING) 0 );                  /* Rexx program output    */

   LOGF(char chTxtBuffer[200];sprintf (chTxtBuffer,"BRORexxFunction::executeRexx() ... %s %d", "Interpreter Return Code: ", rc),chTxtBuffer);

   popRexxContext();

   if (!peekRexxContext())
   {
     LOG("BRORexxFunction::executeRexx() ... Calling RexxWaitForTermination()");
     RexxWaitForTermination();
   }

   LOG("BRORexxFunction::executeRexx() ... done");
   return rc;
};

// Call rexx with arguments but without getting the result - i.e. as a subroutine
int BRORexxFunction::executeRexx(std::vector<std::string>& args)
{
   LOG("BRORexxFunction::executeRexx(args)");
//   IFLOG(!exitlist, "BRORexxFunction::executeRexx(args) ... exitlist is null");
//   IFLOG(!rexx, "BRORexxFunction::executeRexx(args) ... rexx is null");
//   IFLOG(externalrexx.size()==0, "BRORexxFunction::executeRexx(args) ... externalrexx zero length");

   APIRET   rc;                                  /* return code from REXX  */
   PSZ rexxname;

   if (externalrexx.size()==0) rexxname = (PSZ)name.c_str();
   else rexxname = (PSZ)externalrexx.c_str();

   // Handle arguments
   int noArgs = args.size();
   PRXSTRING rexxArgs = 0;
   if (noArgs)
   {
     rexxArgs = new RXSTRING[noArgs];
     for (int i=0; i<noArgs; i++)
       MAKERXSTRING(rexxArgs[i], (char*)args[i].c_str(), args[i].size());
   }

   pushRexxContext(this);

//   LOG("BRORexxFunction::executeRexx(args) ... Calling RexxStart");
   rc=RexxStart((LONG)     noArgs,               /* number of arguments    */
               (PRXSTRING) rexxArgs,             /* array of arguments     */
               (PSZ)       rexxname,             /* name of REXX file      */
               (PRXSTRING )rexx,                 /* INSTORE used           */
               (PSZ)       "RXOSHELL",            /* Command env. name      */
               (LONG)      RXSUBROUTINE,         /* Code for how invoked   */
               (PRXSYSEXIT)exitlist,             /* EXITs on this call     */
               (PSHORT)    0,                    /* Rexx program output    */
               (PRXSTRING) 0 );                  /* Rexx program output    */

//   LOGF(char chTxtBuffer[200];sprintf (chTxtBuffer,"BRORexxFunction::executeRexx(args) ... %s %d", "Interpreter Return Code: ", rc),chTxtBuffer);

   popRexxContext();

   if (!peekRexxContext())
   {
     LOG("BRORexxFunction::executeRexx() ... Calling RexxWaitForTermination()");
     RexxWaitForTermination();
   }

   // Clear up arguments
   if (rexxArgs) delete[] rexxArgs;

//   LOG("BRORexxFunction::executeRexx(args) ... done");
   return rc;
};

// Call rexx with arguments and getting the result - i.e. as a function
int BRORexxFunction::executeRexx(std::vector<std::string>& args, std::string& ret)
{
   LOG("BRORexxFunction::executeRexx(args, ret)");
//   IFLOG(!exitlist, "BRORexxFunction::executeRexx(args, ret) ... exitlist is null");
//   IFLOG(!rexx, "BRORexxFunction::executeRexx(args, ret) ... rexx is null");
//   IFLOG(externalrexx.size()==0, "BRORexxFunction::executeRexx(args, ret) ... externalrexx zero length");

   APIRET   rc;                                  /* return code from REXX  */
   PSZ rexxname;
   RXSTRING rexxretval;                /* return value from REXX     */

   if (externalrexx.size()==0) rexxname = (PSZ)name.c_str();
   else rexxname = (PSZ)externalrexx.c_str();

   /* By setting the strlength of the output RXSTRING to zero, we    */
   /* force the interpreter to allocate memory and return it to us.  */
   /* We could provide a buffer for the interpreter to use instead.  */
   rexxretval.strlength = 0L;          /* initialize return to empty */
   rexxretval.strptr = NULL; // For OOREXX

   // Handle arguments
   int noArgs = args.size();
   PRXSTRING rexxArgs = 0;
   if (noArgs)
   {
     rexxArgs = new RXSTRING[noArgs];
     for (int i=0; i<noArgs; i++)
       MAKERXSTRING(rexxArgs[i], (char*)args[i].c_str(), args[i].size());
   }

   pushRexxContext(this);

//   LOG("BRORexxFunction::executeRexx(args, ret) ... Calling RexxStart");
   rc=RexxStart((LONG)     noArgs,               /* number of arguments    */
               (PRXSTRING) rexxArgs,             /* array of arguments     */
               (PSZ)       rexxname,             /* name of REXX file      */
               (PRXSTRING )rexx,                 /* INSTORE used           */
               (PSZ)       "RXOSHELL",            /* Command env. name      */
               (LONG)      RXFUNCTION,           /* Code for how invoked   */
               (PRXSYSEXIT)exitlist,             /* EXITs on this call     */
               (PSHORT)    0,                    /* Rexx program output    */
               (PRXSTRING) &rexxretval );        /* Rexx program output    */

//   LOGF(char chTxtBuffer[200];sprintf (chTxtBuffer,"BRORexxFunction::executeRexx(args, ret) ... %s %d", "Interpreter Return Code: ", rc),chTxtBuffer);

   popRexxContext();

   if (!peekRexxContext())
   {
     LOG("BRORexxFunction::executeRexx() ... Calling RexxWaitForTermination()");
     RexxWaitForTermination();
   }

   // Clear up arguments
//   LOG("BRORexxFunction::executeRexx(args, ret) ... delete rexxArgs");
   if (rexxArgs) delete[] rexxArgs;

   // Handle Return Value
   if (rexxretval.strptr)
   {
//     LOG("BRORexxFunction::executeRexx(args, ret) ... assign");
     ret.assign(rexxretval.strptr,rexxretval.strlength);

//     LOG("BRORexxFunction::executeRexx(args, ret) ... RexxFreeMemory");
     RexxFreeMemory(rexxretval.strptr);      /* Release storage given to us by REXX */
   }
   else
   {
//     LOG("BRORexxFunction::executeRexx(args, ret) ... clear");
     ret.clear();
   }

//   LOG("BRORexxFunction::executeRexx(args, ret) ... done");
   return rc;
};


// Sets the rexx inline callback information.
// If rexxfunction is blank then it clears the structure and releases memeory
// It also clears the externelrexx function value (if rexxfunction is not blank).
void BRORexxFunction::setRexx(std::string& rexxcd)
{
    if (rexxcode)
    {
      delete rexxcode;
      rexxcode = 0;
    }
    if (rexxcd.size())
    {
      rexxcode = new std::string(rexxcd);
      setRexxWithStatic(rexxcode->c_str());
    }
    else setRexxWithStatic(0);
};


// Sets the rexx inline callback information from a static char* (i.e. code is NOT copied)
// If rexxfunction is null then it clears the structure and releases memeory
// It also clears the externelrexx function value (if rexxfunction is not NULL).
void BRORexxFunction::setRexxWithStatic(const char* rexxcd)
{
  // Clear any memory etc.
  if (rexx)
  {
    if (((PRXSTRING)rexx)[1].strptr)
      RexxFreeMemory(((PRXSTRING)rexx)[1].strptr);
    delete[] (PRXSTRING)rexx;
    rexx=0;
  }

  // Do we need rexxcode?
  if (rexxcode)
  {
    if (rexxcd != rexxcode->c_str())
    {
      delete rexxcode; // We don't need rexxcode
      rexxcode = 0;
    }
  }

  if (rexxcd) // Setup new rexx function
  {
    externalrexx.clear(); // Clear external function name

    PRXSTRING r = new RXSTRING[2]; // New Rexx function structure
    MAKERXSTRING(r[0], (char*)rexxcd, strlen(rexxcd));
    MAKERXSTRING(r[1], 0, 0);
    rexx=r;

    LOG("BRORexxFunction::setRexxWithStatic() Prep to generate tokenised code()");
    IFLOG(!exitlist, "BRORexxFunction::setRexxWithStatic() ... exitlist is null");
    IFLOG(externalrexx.size()==0, "BRORexxFunction::setRexxWithStatic() ... externalrexx zero length");

    APIRET   rc;                                  /* return code from REXX  */
    PSZ rexxname;
    if (externalrexx.size()==0) rexxname = (PSZ)name.c_str();
    else rexxname = (PSZ)externalrexx.c_str();

    RXSTRING p[1]; // Parameters
    MAKERXSTRING(p[0], "//T", 3);

    LOG("BRORexxFunction::setRexxWithStatic() ... Calling RexxStart");
    rc=RexxStart((LONG)     1,                    /* number of arguments    */
                (PRXSTRING) &p,                    /* array of arguments     */
                (PSZ)       rexxname,             /* name of REXX file      */
                (PRXSTRING )rexx,                 /* INSTORE used           */
                (PSZ)       "RXOSHELL",            /* Command env. name      */
                (LONG)      RXCOMMAND,            /* Code for how invoked   */
                (PRXSYSEXIT)exitlist,             /* EXITs on this call     */
                (PSHORT)    0,                    /* Rexx program output    */
                (PRXSTRING) 0 );                  /* Rexx program output    */
    LOGF(char chTxtBuffer[200];sprintf (chTxtBuffer,"BRORexxFunction::setRexxWithStatic() ... %s %d", "Interpreter Return Code: ", rc),chTxtBuffer);
  }
};

void BRORexxFunction::setExternalRexx(std::string& rexxfunction)
{
  externalrexx = rexxfunction;
  if (externalrexx.size()>0) setRexxWithStatic(0);
};

int BRORexxFunction::SetRexxVariable(
    string name, /* Rexx variable to set */
    string& value) /* value to assign */
{
    LOG("BRORexxFunction::SetRexxVariable(name,value)");
    SHVBLOCK block; /* variable pool control block*/
    block.shvcode = RXSHV_SYSET; /* do a set operation*/
    block.shvret=(UCHAR)0; /* clear return code field */
    block.shvnext=(PSHVBLOCK)0; /* no next block */

    /* set variable name string */
    MAKERXSTRING(block.shvname, (char*)name.c_str(), name.size());

    /* set value string */
    MAKERXSTRING(block.shvvalue, (char*)value.c_str(), value.size());
    block.shvvaluelen=value.size();  /* set value length */
    int rc = RexxVariablePool(&block); /* set the variable */
    if (rc == RXSHV_NEWV) rc=0;
    return rc;
}


int BRORexxFunction::SetRexxVariable(
    string nm,           /* Rexx variable stem to set */
    vector<string>& values) /* values to assign */
{
    LOG("BRORexxFunction::SetRexxVariable(name,values)");
    char buffer[10];
    char buffer2[10];
    SHVBLOCK block0;                /* variable pool control block*/
    int len;
    int num = values.size();
    std::string* names=0;
    SHVBLOCK* blocks=0;
    int rc;
    string name0 = nm + "0";

    // set name.O
    sprintf(buffer,"%d",num);
//    itoa(num, buffer, 10);
    len = strlen(buffer);
    block0.shvcode = RXSHV_SYSET;    /* do a set operation*/
    block0.shvret = (UCHAR)0;          /* clear return code field */
    block0.shvnext = (PSHVBLOCK)0;     /* no next block - yet */
    MAKERXSTRING(block0.shvname, (char*)name0.c_str(), name0.size());
    MAKERXSTRING(block0.shvvalue, buffer, len);
    block0.shvvaluelen=len;          /* set value length */

    // set name.1 ... n
    if (num)
    {
      names=new string[num];
      blocks=new SHVBLOCK[num];
      block0.shvnext=blocks;
      for (int i=0; i<num; i++)
      {
        sprintf(buffer2,"%d",i+1);
//        itoa(i+1, buffer2, 10);
        names[i] = nm + buffer2;
        len = values[i].size();
        blocks[i].shvcode = RXSHV_SYSET;    /* do a set operation*/
        blocks[i].shvret=(UCHAR)0;          /* clear return code field */
        blocks[i].shvnext=blocks+i+1;       /* next block */
        MAKERXSTRING(blocks[i].shvname, (char*)names[i].c_str(), names[i].size());
        MAKERXSTRING(blocks[i].shvvalue,(char*)values[i].c_str(), len);
        blocks[i].shvvaluelen=len;          /* set value length */
      }
      blocks[num-1].shvnext=NULL;           /* End of Blocks */
    }

    rc = RexxVariablePool(&block0);  /* set the variables */

    if (names) delete[] names;
    if (blocks) delete[] blocks;

    if (rc == RXSHV_NEWV) rc=0;
    return rc;
}


int BRORexxFunction::SetRexxVariable(
    vector<string>& names,  /* Rexx variable names to set */
    vector<string>& values) /* values to assign */
{
    LOG("BRORexxFunction::SetRexxVariable(names,values)");
    int nlen, vlen;
    int num = names.size();
    SHVBLOCK* blocks=0;
    int rc;

    if (!num) return 0;

    blocks=new SHVBLOCK[num];
    for (int i=0; i<num; i++)
    {
      vlen = values[i].size();
      nlen = names[i].size();
      blocks[i].shvcode = RXSHV_SET;    /* do a set operation*/
      blocks[i].shvret=(UCHAR)0;          /* clear return code field */
      blocks[i].shvnext=blocks+i+1;       /* next block */
      MAKERXSTRING(blocks[i].shvname, (char*)names[i].c_str(), nlen);
      MAKERXSTRING(blocks[i].shvvalue,(char*)values[i].c_str(), vlen);
      blocks[i].shvvaluelen=vlen;          /* set value length */
    }
    blocks[num-1].shvnext=NULL;           /* End of Blocks */

    rc = RexxVariablePool(blocks);  /* set the variables */

    if (blocks) delete[] blocks;

    if (rc == RXSHV_NEWV) rc=0;
    return rc;
}


int BRORexxFunction::GetRexxVariable(string nm, string &value ) /* Rexx variable to get */
{
    LOG("BRORexxFunction::GetRexxVariable(name,value)");
    SHVBLOCK block; /* variable pool control block*/
    block.shvcode = RXSHV_SYFET; /* do a get operation*/
    block.shvret=(UCHAR)0; /* clear return code field */
    block.shvnext=(PSHVBLOCK)0; /* no next block */
    block.shvvalue.strptr = NULL;
    block.shvvalue.strlength = 0;

    /* set variable name string */
    MAKERXSTRING(block.shvname, (char*)nm.c_str(), nm.size());

    int rc = RexxVariablePool(&block); /* get the variable */

    if (block.shvvalue.strptr)
    {
      value.assign(block.shvvalue.strptr, block.shvvalue.strlength);
      RexxFreeMemory(block.shvvalue.strptr);
    }
    else value.clear();

    if (rc == RXSHV_NEWV) rc=0;
    return rc;
}


int BRORexxFunction::GetRexxVariable(string nm, vector<string> &values )
{
    LOG("BRORexxFunction::GetRexxVariable(name,values)");
    string name0 = nm + "0";
    string value0;
    int num;
    string* names=0;
    SHVBLOCK* blocks=0;
    char buffer[10];
    int rc=0;

    // Get stem size
    GetRexxVariable(name0, value0);
    num=atoi(value0.c_str());

    // get name.1 ... num
    values.clear();
    if (num)
    {
      names=new string[num];
      blocks=new SHVBLOCK[num];
      for (int i=0; i<num; i++)
      {
//        itoa(i+1, buffer, 10);
        sprintf(buffer,"%d",i+1);
        names[i] = nm + buffer;
        blocks[i].shvcode = RXSHV_SYFET; /* do a get operation*/
        blocks[i].shvret=(UCHAR)0; /* clear return code field */
        blocks[i].shvnext=blocks+i+1; /* next block */
        blocks[i].shvvalue.strptr = NULL;
        blocks[i].shvvalue.strlength = 0;
        MAKERXSTRING(blocks[i].shvname, (char*)names[i].c_str(), names[i].size());
      }
      blocks[num-1].shvnext=NULL;           /* End of Blocks */

      rc = RexxVariablePool(blocks);        /* get the variables */

      for (int i=0; i<num; i++)
      {
        string value;
        if (blocks[i].shvvalue.strptr)
        {
          value.assign(blocks[i].shvvalue.strptr, blocks[i].shvvalue.strlength);
          RexxFreeMemory(blocks[i].shvvalue.strptr);
        }
        else value.clear();
        values.push_back(value);
      }
      delete[] names;
      delete[] blocks;
    }

    if (rc == RXSHV_NEWV) rc=0;
    return rc;
}


int BRORexxFunction::SetRexxVariable(BROVar* globals)
{
  return SetRexxVariable(globals->name, globals->value);
};


int BRORexxFunction::SetRexxVariable(BROStem* globals)
{
  return SetRexxVariable(globals->name, globals->values);
};


int BRORexxFunction::SetRexxVariable(BROFilter* globals)
{
  return SetRexxVariable(globals->names, globals->values);
};


int BRORexxFunction::GetRexxVariable(BROVar* globals)
{
  return GetRexxVariable(globals->name, globals->value);
};


int BRORexxFunction::GetRexxVariable(BROStem* globals)
{
  return GetRexxVariable(globals->name, globals->values);
};


int BRORexxFunction::GetRexxVariable(BROFilter* globals)
{
  string name, value;

  SHVBLOCK block; /* variable pool control block*/
  block.shvcode = RXSHV_NEXTV; /* do a get variable operation*/
  block.shvnext=(PSHVBLOCK)0; /* no next block */
  block.shvret=(UCHAR)0; /* clear return code field */

  globals->names.clear();
  globals->values.clear();

  unsigned int filterlen = globals->filter.length();
  const char *filter = globals->filter.c_str();

  while (1)
  {
    block.shvname.strptr = NULL;
    block.shvname.strlength = 0;
    block.shvvalue.strptr = NULL;
    block.shvvalue.strlength = 0;

    int rc = RexxVariablePool(&block); /* get the variable */

    if (rc == RXSHV_LVAR) break;
    else if (rc)
    {
      if (block.shvname.strptr) RexxFreeMemory(block.shvname.strptr);
      if (block.shvvalue.strptr) RexxFreeMemory(block.shvvalue.strptr);
      return rc;
    }

    if (block.shvname.strptr)
    {
      if (filterlen)
      {
        if ((block.shvname.strlength < filterlen) || memcmp(block.shvname.strptr, filter, filterlen))
        {
          RexxFreeMemory(block.shvname.strptr);
          if (block.shvvalue.strptr) RexxFreeMemory(block.shvvalue.strptr);
          continue;
        }
      }
      name.assign(block.shvname.strptr, block.shvname.strlength);
      RexxFreeMemory(block.shvname.strptr);
    }
    else
    {
      LOG("BRORexxFunction::SetRexxVariable(BROFilter* globals). Error - variable name not set");
      return -1;
    }

    if (block.shvvalue.strptr)
    {
      value.assign(block.shvvalue.strptr, block.shvvalue.strlength);
      RexxFreeMemory(block.shvvalue.strptr);
    }
    else value.clear();

    globals->names.push_back(name);
    globals->values.push_back(value);
  }
  return 0;
};


bool BRORexxFunction::isValidFilter(std::string& word)
{
  int len = word.size();
  if (!len) return true; // Empty String Valid

  if (!isalpha(word[0])) return false;

  for (int i=1; i<len; i++)
    if (!isalnum(word[i])) return false;

  return true;
}


bool BRORexxFunction::isValidStem(std::string& word)
{
  int len = word.size();
  if (!len) return false;

  if (word[len-1]!='.') return false;

  if (!isalpha(word[0])) return false;

  for (int i=1; i<len-1; i++)
    if (!isalnum(word[i])) return false;

  return true;
}


bool BRORexxFunction::isValidVar(std::string& word)
{
  int len = word.size();
  if (!len) return false;

  if (!isalpha(word[0])) return false;

  for (int i=1; i<len; i++)
    if (!isalnum(word[i])) return false;

  return true;
}
