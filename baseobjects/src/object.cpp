// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : object.cpp
// Description : Implementation of main/base object
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
#include "object.h"
#include "rexxfunc.h"

#include <stdlib.h>                    /* for strlen()               */
#include <ctype.h>                     /* for toupper() */
#include <sstream>

#ifdef WIN32
#include <windows.h>
#include <process.h> // for _beginthreadex
#else
#include <pthread.h>
#include <errno.h>
#endif


#ifndef WIN32
static std::string UnixError(std::string context)
{
    char sRC[10];
    sprintf(sRC, "%d", errno);
    std::string errorText = context + " RC=" + sRC + " (" + (char*)strerror(errno) + ")";
    return errorText;
};
#endif


/********************************************************************/
/* BROConsoleIO Functions                                           */
/********************************************************************/

// Console Routine
void BROConsoleIO::LineOut(std::string text)
{
//    std::cout << "OUT> " << text << std::endl << std::flush;
    std::cout << text << std::endl << std::flush;
};

// Console Routine
void BROConsoleIO::ErrorOut(std::string text)
{
//    std::cerr << "ERR> " << text << std::endl << std::flush;
    std::cerr << text << std::endl << std::flush;
};

// Console Routine
void BROConsoleIO::TraceOut(std::string text)
{
//    std::cerr << "TRA> " << text << std::endl << std::flush;
    std::cerr << text << std::endl << std::flush;
};

// Console Routine
void BROConsoleIO::GetInput(std::string &entered)
{
    std::getline(std::cin, entered);
};

/********************************************************************/
/* BROBase Console Handling                                       */
/********************************************************************/
void BROBase::LineOut(std::string text)
{
    consoleIO->LineOut(text);
};

// Console Routine
void BROBase::ErrorOut(std::string text)
{
    consoleIO->ErrorOut(text);
};

// Console Routine
void BROBase::TraceOut(std::string text)
{
    consoleIO->TraceOut(text);
};

// Console Routine
void BROBase::GetInput(std::string &entered)
{
    consoleIO->GetInput(entered);
};

int BROStreamBuffer::sync()
{
    std::string empty;
    OutFunc(str());
    str(empty);
    return std::stringbuf::sync();
}

BROStream BROBase::lineOut(BROBase::LineOut);

BROStream BROBase::errorOut(BROBase::ErrorOut);

BROStream BROBase::traceOut(BROBase::TraceOut);

/********************************************************************/
/* Internal Thread Functions                                        */
/********************************************************************/

/* Class to pass parameters to start rexx threads */
class BROThreadParam
{
public:
    std::string func;
    std::vector<std::string> args;
};

/* Class to pass parameters to start C++ threads */
class BROInternalThreadParam
{
public:
    BROBase* object;
    THREAD_FUNCTION func;
    void* arg;
};


// Wrapper Function for unix/pthread/win32 thread signature
#ifdef WIN32
static void ThreadWrapper(void *param)
#else
static void* ThreadWrapper(void *param)
#endif
{
    LOG("static ThreadWrapper()");
    BROInternalThreadParam* val = (BROInternalThreadParam*)param;
    (val->func)(val->object, val->arg);
    delete val;
#ifndef WIN32
    return 0;
#endif
};

/********************************************************************/
/* BROBase Functions                                              */
/********************************************************************/

bool BROBase::ObjectInitted = false;

// Static Map for rexx functions supported by the object
std::map<std::string, BROBase_FUNCTION> BROBase::BROBase_functions;

// Static Map for root (i.e. with no parents) objects
std::map<std::string, BROBase*> BROBase::roots;

// Pointer to overall thread lock - e.g. a CritcalSection in WIN32
void* BROBase::staticMutex = 0;

// Main thread ID
unsigned long BROBase::mainThreadId = 0;

// ConsoleIO Helper class
BROConsoleIO* BROBase::consoleIO = 0;

// A thread variable holding a pointer to a thread [specific] object
void* BROBase::threadIndex = 0;

// Holds the full path if the exe
std::string BROBase::exeName = "";

void BROBase::InitObject()
{
    InitObject("");
}

void BROBase::InitObject(std::string _exeName)
{
    if (ObjectInitted) return;
    createCriticalSection(staticMutex);
    enterStaticCriticalSection();
    ObjectInitted = true;

    // Assume this is the main thread ...
#ifdef WIN32
    mainThreadId = GetCurrentThreadId();
#else
    mainThreadId = pthread_self();
#endif

#ifdef WIN32
    threadIndex = (void*)(new DWORD);
    *(DWORD*)threadIndex = TlsAlloc();
#else
    threadIndex = (void*)(new pthread_key_t);
    pthread_key_create((pthread_key_t*)threadIndex, NULL);
#endif

    // Set Exe Name
    exeName = _exeName;

    // Setup Console IO Object
    if (!consoleIO) consoleIO = new BROConsoleIO;

    // Set-up rexx function mapping - static - only done once
    BROBase_functions["CALLFUNCTION"] = BROBase::staticCallFunction;
    BROBase_functions["CALLPROCEDURE"] = BROBase::staticCallProcedure;
    BROBase_functions["GETPARENT"] = BROBase::staticGetParent;
    BROBase_functions["FINDCHILD"] = BROBase::staticFindChild;
    BROBase_functions["LISTCHILDREN"] = BROBase::staticListChildren;
    BROBase_functions["FINDOBJECT"] = BROBase::staticFindObject;
    BROBase_functions["FINDROOT"] = BROBase::staticFindRoot;
    BROBase_functions["LISTROOT"] = BROBase::staticListRoot;
    BROBase_functions["GETTYPE"] = BROBase::staticGetType;
    BROBase_functions["SETEXTERNALREXX"] = BROBase::staticSetExternalRexx;
    BROBase_functions["SETREXX"] = BROBase::staticSetRexx;
    BROBase_functions["GETNAME"] = BROBase::staticGetName;
    BROBase_functions["GETEXENAME"] = BROBase::staticGetExeName;
    BROBase_functions["GETFULLNAME"] = BROBase::staticGetFullName;
    BROBase_functions["NEWOBJECT"] = BROBase::staticNewObject;
    BROBase_functions["NEWROOTOBJECT"] = BROBase::staticNewRootObject;
    BROBase_functions["ADDGLOBALVAR"] = BROBase::staticAddGlobalVar;
    BROBase_functions["ADDGLOBALSTEM"] = BROBase::staticAddGlobalStem;
    BROBase_functions["ADDGLOBALFILTER"] = BROBase::staticAddGlobalFilter;
    BROBase_functions["ISVALIDVAR"] = BROBase::staticIsValidVar;
    BROBase_functions["ISVALIDSTEM"] = BROBase::staticIsValidStem;
    BROBase_functions["ISVALIDFILTER"] = BROBase::staticIsValidFilter;
    BROBase_functions["LOADVAR"] = BROBase::staticLoadVar;
    BROBase_functions["SAVEVAR"] = BROBase::staticSaveVar;
    BROBase_functions["LOADSTEM"] = BROBase::staticLoadStem;
    BROBase_functions["SAVESTEM"] = BROBase::staticSaveStem;
    BROBase_functions["LOADFILTER"] = BROBase::staticLoadFilter;
    BROBase_functions["SAVEFILTER"] = BROBase::staticSaveFilter;
    BROBase_functions["ISCHILDTHREAD"] = BROBase::staticIsChildThread;
    BROBase_functions["ISMAINTHREAD"] = BROBase::staticIsMainThread;
    BROBase_functions["REXXFUNCEXISTS"] = BROBase::staticRexxFuncExists;
    BROBase_functions["STARTTHREAD"] = BROBase::staticStartThread;
    BROBase_functions["DELETEOBJECT"] = BROBase::staticDeleteObject;

    BRORexxFunction::InitRexx();

    leaveStaticCriticalSection();
};


void BROBase::DoneWithObject()
{
    if (!ObjectInitted) return;
    enterStaticCriticalSection();
    if (!ObjectInitted)
    {
        leaveStaticCriticalSection();
        return;
    }

    /* Delete any objects */
    while (!roots.empty())
        delete (*(roots.begin())).second;

    BRORexxFunction::DoneWithRexx();


    // Delete Thread Object(s)
    ThreadObject* threadObject;
#ifdef WIN32
    threadObject = (ThreadObject*)TlsGetValue(*(DWORD*)threadIndex);
#else
    threadObject = (ThreadObject*)pthread_getspecific(*(pthread_key_t*)threadIndex);
#endif
    if (threadObject)
    {
        delete threadObject;
#ifdef WIN32
        TlsSetValue(*(DWORD*)threadIndex, 0);
#else
        pthread_setspecific(*(pthread_key_t*)threadIndex, 0);
#endif
    }

#ifdef WIN32
    TlsFree(*(DWORD*)threadIndex);
    delete (DWORD*)threadIndex;
#else
    pthread_key_delete(*(pthread_key_t*)threadIndex);
    delete (pthread_key_t*)threadIndex;
#endif

    if  (consoleIO) delete consoleIO;

    ObjectInitted = false;

    leaveStaticCriticalSection();
    deleteCriticalSection(staticMutex);
};


/* Enters the critcal section if the object is not a zombie */
/* returns 0 on sucess */
/* returns -1 if the object is a zombie */
/* Nb. If you need both an object and a static Mutex - */
/*   ALWAYS get the STATIC one FIRST */
/*   ALWAYS lock a parent object before a child */
/*   AVOID locking two sibling objects at the same time */
int BROBase::enterObjectCriticalSectionIfNotZombie()
{
    if (zombie) return -1;
    enterObjectCriticalSection();
    if (zombie)
    {
        leaveObjectCriticalSection();
        return -1;
    }
    return 0;
};


/* Nb. If you need both an object and a static Mutex - */
/*   ALWAYS get the STATIC one FIRST */
/*   ALWAYS lock a parent object before a child */
/*   AVOID locking two sibling objects at the same time */
void BROBase::enterObjectCriticalSection()
{
    enterCriticalSection(objectMutex);
};


void BROBase::leaveObjectCriticalSection()
{
    leaveCriticalSection(objectMutex);
};


/* Nb. If you need both an object and a static Mutex - */
/*   ALWAYS lock the STATIC one FIRST */
/*   ALWAYS lock a parent object before a child */
/*   AVOID locking two sibling objects at the same time */
#ifdef _DEBUG
static int scr_debug=0;
#endif

void BROBase::enterStaticCriticalSection()
{
    LOG("BROBase::enterStaticCriticalSection()");
#ifdef _DEBUG
    scr_debug++;
    if (scr_debug>1) std::cerr << "BROBase::enterStaticCriticalSection(): Depth=" << scr_debug << std::endl << std::flush;
#endif
    enterCriticalSection(staticMutex);
}


void BROBase::leaveStaticCriticalSection()
{
    LOG("BROBase::leaveStaticCriticalSection()");
#ifdef _DEBUG
    scr_debug--;
    if (scr_debug) std::cerr << "BROBase::leaveStaticCriticalSection(): Depth=" << scr_debug << std::endl << std::flush;
#endif
    leaveCriticalSection(staticMutex);
}


void BROBase::enterCriticalSection(void* mutex)
{
    if (mutex)
    {
#ifdef WIN32
        EnterCriticalSection((LPCRITICAL_SECTION)mutex);
#else
        pthread_mutex_lock((pthread_mutex_t*)mutex);
#endif
    }
};


void BROBase::leaveCriticalSection(void* mutex)
{
    if (mutex)
    {
#ifdef WIN32
        LeaveCriticalSection((LPCRITICAL_SECTION)mutex);
#else
        pthread_mutex_unlock((pthread_mutex_t*)mutex);
#endif
    }
};


void BROBase::createCriticalSection(void* &mutex)
{
#ifdef WIN32
    mutex = new CRITICAL_SECTION;
    InitializeCriticalSection((LPCRITICAL_SECTION)mutex);
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    mutex = new pthread_mutex_t;
    if (pthread_mutex_init((pthread_mutex_t*)mutex, &attr))
    {
        throw BROCriticalSectionError(UnixError("Failure pthread_mutex_init()"));
    }
    pthread_mutexattr_destroy(&attr);
#endif
}


void BROBase::deleteCriticalSection(void* &mutex)
{
    if (mutex) // Should always be the case
    {
        void* temp = mutex;
        mutex = NULL;
#ifdef WIN32
        // Note: no defined errors for Win32 Critical section API
        leaveCriticalSection(temp);
        DeleteCriticalSection((LPCRITICAL_SECTION)temp);
        delete (LPCRITICAL_SECTION)temp;
#else
        pthread_mutex_unlock((pthread_mutex_t*)temp);
        pthread_mutex_destroy((pthread_mutex_t*)temp);
        delete (pthread_mutex_t*)temp;
#endif
    }
}


// Returns true if rexx is running on this thread
bool BROBase::rexxRunning()
{
    if (BRORexxFunction::peekRexxContext()) return true;
    return false;
}


// Common code for all constructors */
void BROBase::BROBase_setup(BROBase* pnt, std::string nm)
{
    if (!ObjectInitted) InitObject();

    zombie = false;

    createCriticalSection(objectMutex);

    type = "BROBase";
    parent = pnt;
    upperCase(nm);
    name = nm;

    if (parent) // Add me as a child of my parent
    {
        if (parent->enterObjectCriticalSectionIfNotZombie())
        {
            // Mmm the parent is being deleted
            BROBase_kill(true);
            throw BROBaseNoLongerExistsError("Parent object");
        }
        if (parent->children.count(name)>0)
        {
            parent->leaveObjectCriticalSection();
            BROBase_kill(true);
            throw BRODuplicateChildName(name);
        }
        parent->children[name] = this;
        parent->leaveObjectCriticalSection();
    }

    else // Add me as a root item
    {
        enterStaticCriticalSection();
        if (roots.count(name)>0)
        {
            leaveStaticCriticalSection();
            BROBase_kill(true);
            throw BRODuplicateRootName(name);
        }
        roots[name] = this;
        leaveStaticCriticalSection();
    }
}


// Constructor
BROBase::BROBase(BROBase* pnt, std::string nm)
{
    BROBase_setup(pnt, nm);
};


// Constructor with extra args from REXX (which are ignored in this case)
BROBase::BROBase(BROBase* pnt, std::string nm, std::vector<std::string> &args)
{
    BROBase_setup(pnt, nm);
};


// Common Destructor
void BROBase::BROBase_kill(bool propagate)
{
    LOG("BROBase::BROBase_kill() name=" + name + " type=" + type);

    // OK Some complex logic to try to avoid two threads deleting
    // the same object and/or a deadlock
    if (zombie) return;

    /* Note for below - the parent could very well be a zombie deleting its
       children (including this object) */
    if (parent) parent->enterObjectCriticalSection();
    else enterStaticCriticalSection();

    enterObjectCriticalSection();

    if (zombie) // Bad luck (and timing)
    {
        leaveObjectCriticalSection();
        if (parent) parent->leaveObjectCriticalSection();
        else leaveStaticCriticalSection();
        return;
    }
    zombie = true;

    if (parent) // Remove me as a child of my parent
    {
        parent->children.erase(name);
        parent->leaveObjectCriticalSection();
    }
    else
    {
        roots.erase(name); // Remove me form the list of root items
        leaveStaticCriticalSection();
    }
    // We now have removed ourselves from the parent and
    // have locked out the object

    // Now delete any children
    while (!children.empty())
        delete (*(children.begin())).second;

    // Delete functions
    while (!RexxFunctions.empty())
    {
        std::map<std::string, BRORexxFunction*>::iterator i = RexxFunctions.begin();
        delete i->second;
        RexxFunctions.erase(i);
    }

    // Delete Globals
    while (!globalVars.empty())
    {
        std::map<std::string, BROVar*>::iterator i = globalVars.begin();
        delete i->second;
        globalVars.erase(i);
    }
    while (!globalStems.empty())
    {
        std::map<std::string, BROStem*>::iterator i = globalStems.begin();
        delete i->second;
        globalStems.erase(i);
    }
    while (!globalFilters.empty())
    {
        std::map<std::string, BROFilter*>::iterator i = globalFilters.begin();
        delete i->second;
        globalFilters.erase(i);
    }

    // Delete the critical section - if it was set-up
    leaveObjectCriticalSection();
    deleteCriticalSection(objectMutex);
    LOG("BROBase::BROBase_kill() - Done");
};


// Destructor
BROBase::~BROBase()
{
    BROBase_kill(false);
};


int BROBase::staticIsMainThread(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        ret="";
        setError(1, "Invalid Arguments - should be no arguments");
        return 1; // Invalid Arguments
    }
    else
    {
        if (isMainThread()) ret="1";
        else ret="0";
    };
    return 0;
};


bool BROBase::isMainThread()
{
#ifdef WIN32
    unsigned long myThread = GetCurrentThreadId();
#else
    unsigned long myThread = pthread_self();
#endif
    return (myThread == mainThreadId);
};


int BROBase::staticIsChildThread(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        ret="";
        setError(1, "Invalid Arguments - should be no arguments");
        return 1; // Invalid Arguments
    }
    else
    {
        if (isChildThread()) ret="1";
        else ret="0";
    };
    return 0;
};


bool BROBase::isChildThread()
{
    return !isMainThread();
}


void BROBase::upperCase(std::string &word)
{
    for (int i=0; i<(int)word.size(); i++)
    {
        word[i] = toupper(word[i]);
    }
};


// Delete an object
// TODO - Make sure object and children are not busy (i.e. running rexx) by adding a counter for each object indicating
// that the object is busy
int BROBase::staticDeleteObject(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    ret="";
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - should be OBJECT");
        return 1;
    }

    BROBase* c=object->findObject(args[0]);
    if (!c)
    {
        setError(2, "Object Not Found: " + args[0]);
        return 2;
    }

    delete c;
    setError(0,"");
    return 0;
};


int BROBase::staticFindObject(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - should be OBJECT");
        return 1; // Invalid Arguments
    }
    else
    {
        BROBase* c=object->findObject(args[0]);
        if (c) c->getFullName(ret);
        else ret="";
    };
    return 0;
};


BROBase* BROBase::findObject(std::string& target)
{
    LOG("BROBase::findObject(" + target + ")");
    upperCase(target);
    std::string first;
    std::string rest;
    if (target.size())
    {
        if (target=="SELF") return this;

        else if (target=="PARENT")
        {
            return getParent();
        }

        else if (target.substr(0,1) == ".")
        {
            std::string path=target.substr(1);
            getFirstPart(path, first, rest);
            BROBase* obj = findRoot(first);
            if (obj) return obj->findObject(rest);
            else return 0;
        }

        else
        {
            getFirstPart(target, first, rest);
            if (first=="SELF") return findObject(rest);
            else if (first=="PARENT")
            {
                BROBase* p = getParent();
                if (p) return p->findObject(rest);
                else return 0;
            }
            else
            {
                BROBase* obj = findChild(first);
                if (obj) return obj->findObject(rest);
                else return 0;
            }
        }
    };

    // No target specified - so must be this object
    return this;
};


// Calls the static version of the function called by rexx on the current object
// returns -1 if the function is not found
//         -99 if the object is a zombie
//         0 Success
//         >0 Function Specific Error
int BROBase::functionHandler(std::string name, std::vector<std::string> &args, std::string &ret)
{
    LOGF(std::string n; getName(n),"BROBase(" + n + ")::functionHandler(" + name + ", args, ret)");
//  enterObjectCriticalSection();

//  if (zombie)
//  {
//    leaveObjectCriticalSection();
//    return -99;
//  }
    upperCase(name);
    BROBase_FUNCTION func = 0;

    std::map<std::string, BROBase_FUNCTION>::iterator iter = BROBase_functions.find(name);
    if ( iter!=BROBase_functions.end() ) func = iter->second;

//  leaveObjectCriticalSection();
    int rc;
    if (func)
    {
        rc = func(this, args, ret); // 0 = success. Positive Number = function specific error
        if (rc<0) rc = -rc; // Make sure it is postive - i.e. not to be confused with function not found
    }

    else rc = executeRexx(name, args, ret);

    return rc;
}


// Calls the static version of the function called by rexx on the current object
// returns -1 if the function is not found
//         -99 if the object is a zombie
int BROBase::procedureHandler(std::string name, std::vector<std::string> &args)
{
    LOGF(std::string n; getName(n),"BROBase(" + n + ")::procedureHandler(" + name + ", args)");
//  enterObjectCriticalSection();

//  if (zombie)
//  {
//    leaveObjectCriticalSection();
//    return -99;
//  }
    upperCase(name);
    BROBase_FUNCTION func = 0;

    std::map<std::string, BROBase_FUNCTION>::iterator iter = BROBase_functions.find(name);
    if ( iter!=BROBase_functions.end() ) func = iter->second;


//  leaveObjectCriticalSection();
    int rc;
    std::string ret;
    if (func)
    {
        rc = func(this, args, ret); // 0 = success. Positive Number = function specific error
        if (rc<0) rc = -rc; // Make sure it is postive - i.e. not to be confused with function not found
    }

    else rc = executeRexx(name, args);

    return rc;
}


// Static [meta] function called by a rexx program to call another procedure
int BROBase::staticCallProcedure(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::staticCallProcedure(object, args, ret)");
    int rc;
    ret="";

    if (!object)
    {
        setError(99, "Internal Error - Object not set");
        return 99;
    }

    if (args.size()==2)
    {
        std::vector<std::string> nothing;
        rc = object->callProcedure(args[0], args[1], nothing);
    }
    else if (args.size()>2)
    {
        // Get a vector of the rest of the arguments
        std::vector<std::string> rest((args.begin())+=2, args.end());
        rc = object->callProcedure(args[0], args[1], rest);
    }
    else
    {
        setError(1, "Invalid Arguments - should be TARGET, FUNCTION, [Function Args], ...");
        return 1; // Less than two arguments ...
    }

    if (rc<0)
    {
        setError(2, "Either Target object (" + args[0] + ") or Function (" + args[1] + ") not found");
        return 2;
    }
    else if (rc>0)
    {
        std::string rcode;
        std::stringstream tmp;
        tmp << rc;
        rcode = tmp.str();
        setError(3, "Function Specific Error Code: " + rcode);
        return 3;
    }

    setError(0, "");
    return 0;
};


// Static [meta] function called by a rexx program to call another function
int BROBase::staticCallFunction(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::staticCallFunction(object, args, ret)");
    int rc;

    if (!object)
    {
        setError(99, "Internal Error - Object not set");
        return 99;
    }

    if (args.size()==2)
    {
        std::vector<std::string> nothing;
        rc = object->callFunction(args[0], args[1], nothing, ret);
    }
    else if (args.size()>2)
    {
        // Get a vector of the rest of the arguments
        std::vector<std::string> rest((args.begin())+=2, args.end());
        rc = object->callFunction(args[0], args[1], rest, ret);
    }
    else
    {
        setError(1, "Invalid Arguments - should be TARGET, FUNCTION, [Function Args], ...");
        return 1; // Less than two arguments ...
    }

    if (rc<0)
    {
        setError(2, "Either Target object (" + args[0] + ") or Function (" + args[1] + ") not found");
        return 2;
    }
    else if (rc>0)
    {
        std::string rcode;
        std::stringstream tmp;
        tmp << rc;
        rcode = tmp.str();
        setError(3, "Function Specific Error Code: " + rcode);
        return 3;
    }

    setError(0, "");
    return 0;
};


// Strips the first part off the path */
void BROBase::getFirstPart(std::string& path, std::string& first, std::string& rest)
{
    int pos = path.find('.');
    if (pos == (int)std::string::npos)
    {
        first = path;
        rest.clear();
    }
    else
    {
        first = path.substr(0, pos);
        rest = path.substr(pos+1);
    }
};


// Strips the last part off a path
void BROBase::getLastPart(std::string& path, std::string& last, std::string& rest)
{
    int pos = path.rfind('.');
    if (pos == (int)std::string::npos)
    {
        last = path;
        rest.clear();
    }
    else
    {
        last = path.substr(pos+1);
        rest = path.substr(0, pos);
    }
};


// Strips the stem name off a path
void BROBase::getLastStem(std::string& path, std::string& stem, std::string& rest)
{
    int pos;
    int lastpos = path.size()-1;

    if (path[lastpos]=='.') pos = (path.substr(0,lastpos)).rfind('.');
    else pos = path.rfind('.');

    if (pos == (int)std::string::npos)
    {
        stem = path;
        rest.clear();
    }
    else
    {
        stem = path.substr(pos+1);
        rest = path.substr(0, pos);
    }
};


// Finds and calls a function on a target object
int BROBase::callFunction(std::string target, std::string name, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::callFunction(target, name, args, ret)");
    LOG("BROBase::callFunction(target, name, args, ret)  ... target=" + target);
    LOG("BROBase::callFunction(target, name, args, ret)  ... name=" + name);
    BROBase* obj;

    obj = findObject(target);
    if (obj) return obj->functionHandler(name, args, ret);
    else return -1;
};


// Finds and calls a function on a target object
int BROBase::callProcedure(std::string target, std::string name, std::vector<std::string> &args)
{
    LOG("BROBase::callProcedure(" + target + ", " + name + ", args)");
    BROBase* obj;

    obj = findObject(target);
    if (obj) return obj->procedureHandler(name, args);
    else return -1;
};


// Finds and calls a function on a target object
int BROBase::callFunction(std::string target_and_name, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::callFunction(target_and_name, args, ret)");
    LOG("BROBase::callFunction(target_and_name, args, ret)  ... target_and_name=" + target_and_name);

    upperCase(target_and_name);

    std::string target;
    std::string name;

    getLastPart(target_and_name, name, target);

    return callFunction(target, name, args, ret);
};


// Finds and calls a procedure on a target object
int BROBase::callProcedure(std::string target_and_name, std::vector<std::string> &args)
{
    LOG("BROBase::callProcedure(" + target_and_name + ", args)");

    upperCase(target_and_name);

    std::string target;
    std::string name;

    getLastPart(target_and_name, name, target);

    return callProcedure(target, name, args);
};


// Start a new thread
int BROBase::startThread(THREAD_FUNCTION function, void* param)
{
    LOG("BROBase::startThread()");
    BROInternalThreadParam* val = new BROInternalThreadParam;
    val->object = this;
    val->func = function;
    val->arg = param;
#ifdef WIN32
    _beginthread(ThreadWrapper , 0, (void*)val);
#else
    pthread_t threadID;
    if (pthread_create(&threadID, NULL, ThreadWrapper, (void*)val)) return -1;
#endif
    return 0;
}


// Start a new thread
int BROBase::staticStartThread(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::staticStartThread(): " + args[0]);
    if (args.size() < 1)
    {
        setError(1, "Invalid Arguments - should be FUNCTION, [Function Args], ...");
        return 1; // Less than one arguments ...
    }
    else
    {
        std::string func=args[0];
        upperCase(func);
        if (object->enterObjectCriticalSectionIfNotZombie())
        {
            setError(3, "Target object is being destroyed by another thread");
            return 3;
        }

        BRORexxFunction* f=0;

        std::map<std::string,BRORexxFunction*>::iterator iter = object->RexxFunctions.find(func);
        if ( iter!=object->RexxFunctions.end() ) f = iter->second;

        object->leaveObjectCriticalSection();
        if (!f)
        {
            setError(2, "Function not found: " + func);
            return 2;
        }
        BROThreadParam* val = new BROThreadParam;
        val->func = func;
//        if (args.size()>1)
//        {
            // Get a vector of the rest of the arguments
            std::vector<std::string> rest((args.begin())++, args.end());
            val->args = rest;
//        }
        if (object->startThread(ThreadMain, (void*)val))
        {
            setError(4, "Internal Error - could not start thread");
            return 4;
        }
        ret = "0";
        return 0;
    };
}


// Static Wrapper Function to call a rexx function
void BROBase::ThreadMain(BROBase* object, void *param)
{
    LOG("BROBase::ThreadMain()");

    // TODO - Create/finish thread objects?
    BROThreadParam* val = (BROThreadParam*)param;
    int rc = object->executeRexx(val->func, val->args);
    delete val;

    LOG("BROBase::ThreadMain() rc="+rc);


    // Delete Thread Object(s)
    ThreadObject* threadObject;
#ifdef WIN32
    threadObject = (ThreadObject*)TlsGetValue(*(DWORD*)threadIndex);
#else
    threadObject = (ThreadObject*)pthread_getspecific(*(pthread_key_t*)threadIndex);
#endif
    if (threadObject)
    {
        delete threadObject;
#ifdef WIN32
        TlsSetValue(*(DWORD*)threadIndex, 0);
#else
        pthread_setspecific(*(pthread_key_t*)threadIndex, 0);
#endif
    }
};

ThreadObject* BROBase::getThreadObject()
{
    ThreadObject* threadObject;
#ifdef WIN32
    threadObject = (ThreadObject*)TlsGetValue(*(DWORD*)threadIndex);
#else
    threadObject = (ThreadObject*)pthread_getspecific(*(pthread_key_t*)threadIndex);
#endif
    if (!threadObject)
    {
        threadObject = new ThreadObject();
#ifdef WIN32
        TlsSetValue(*(DWORD*)threadIndex, (void*)threadObject);
#else
        pthread_setspecific(*(pthread_key_t*)threadIndex, (void*)threadObject);
#endif
    }
    return threadObject;
}


// Calls a rexx function associated with this object
// returns -1 if function not found, -99 for zombie
int BROBase::executeRexx(std::string function)
{
    LOG("BROBase::executeRexx(): " + function);
    if (enterObjectCriticalSectionIfNotZombie()) return -99;

    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    leaveObjectCriticalSection();

    if (!func) return -1;

    int rc=func->executeRexx();

    if (rc<0) rc=-rc; // So as not to be confused with -1 = not found

    return rc;
};


// Calls a rexx function associated with this object
// returns -1 if function not found, -99 for zombie
int BROBase::executeRexx(std::string function, std::vector<std::string>& args)
{
    LOG("BROBase::executeRexx(): " + function);

    if (enterObjectCriticalSectionIfNotZombie()) return -99;

    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    leaveObjectCriticalSection();

    if (!func) return -1;

    int rc=func->executeRexx(args);

    if (rc<0) rc=-rc; // So as not to be confused with -1 = not found

    return rc;
};


// Calls a rexx function associated with this object
// returns -1 if function not found, -99 for zombie
int BROBase::executeRexx(std::string function, std::vector<std::string>& args, std::string& ret)
{
    LOG("BROBase::executeRexx()");
    if (enterObjectCriticalSectionIfNotZombie()) return -99;

    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    leaveObjectCriticalSection();

    if (!func) return -1;

    int rc=func->executeRexx(args, ret);

    if (rc<0) rc=-rc; // So as not to be confused with -1 = not found

    return rc;
};


// Creates a new child opject of the target.
// Arg 1 - new object type
// Arg 2 - new object name
// sets ret to "0" on sucess
// sets ret to "rc error message" on failure
int BROBase::staticNewObject(BROBase* target, std::vector<std::string> &args, std::string &ret)
{
    BROBase* obj=0;
    if (args.size()<2)
    {
        ret="1 Invalid arguments - should be (new_object_type, new_object_name)";
        return 0;
    }
    try
    {
        if (args.size()==2) obj = target->newObject(args[0], args[1]);
        else
        {
            // We understand the first two arguments
            std::string tp = args[0];
            std::string nm = args[1];
            // Get a vector of the rest of the arguments
            std::vector<std::string> rest((args.begin())+=2, args.end());
            obj = target->newObject(tp, nm, rest);
        }
    }
    catch (std::exception &ex)
    {
        ret=std::string("2 ") + ex.what();
        return 0;
    }

    if (!obj) ret="3 Unknown Internal Error";
    else ret="0";

    return 0;
};


// Creates a new root object
// Arg 1 - new object type
// Arg 2 - new object name
// sets ret to "0" on sucess
// sets ret to "rc error message" on failure
int BROBase::staticNewRootObject(BROBase* target, std::vector<std::string> &args, std::string &ret)
{
    BROBase* obj=0;
    if (args.size()<2)
    {
        ret="1 Invalid arguments - should be (new_object_type, new_object_name)";
        return 0;
    }
    try
    {
        if (args.size()==2)
          obj = BROBaseFactory::newObject(0, args[0], args[1]);

        else
        {
            // We understand the first two arguments
            std::string tp = args[0];
            std::string nm = args[1];
            // Get a vector of the rest of the arguments
            std::vector<std::string> rest((args.begin())+=2, args.end());

            obj = BROBaseFactory::newObject(0, tp, nm, rest);
        }
    }
    catch (std::exception &ex)
    {
        ret=std::string("2 ") + ex.what();
        return 0;
    }

    if (!obj) ret="3 Unknown Internal Error";
    else ret="0";

    return 0;
};

// Creates a new child object - with some extra aruments
BROBase* BROBase::newObject(std::string objectType, std::string objectName, std::vector<std::string> &args)
{
    return BROBaseFactory::newObject(this, objectType, objectName, args);
};


// Creates a new child object
BROBase* BROBase::newObject(std::string objectType, std::string objectName)
{
    return BROBaseFactory::newObject(this, objectType, objectName);
};


// Called by the factory before adding a child
void BROBase::beginNewObject() {};


// Called by the factory after adding a child
void BROBase::endNewObject() {};


// Returnes the full name of the parent object (or blank if a root)
int BROBase::staticGetParent(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        setError(1, "Invalid Arguments - should be no arguments");
        return 1;
    }
    else
    {
        BROBase* p=object->getParent();
        if (p) p->getFullName(ret);
        else ret="";
        return 0;
    }
}


// Returns a pointer to the parent
BROBase* BROBase::getParent()
{
    enterObjectCriticalSection();
    BROBase* ret=parent;
    leaveObjectCriticalSection();
    return ret;
}


// Returns the full name of a child (or blank if the child does not exist
int BROBase::staticFindChild(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - should be OBJECT");
        return 1; // Invalid Arguments
    }
    else
    {
        BROBase* c=object->findChild(args[0]);
        if (c) c->getFullName(ret);
        else ret="";
    };
    return 0;
}


// Returns the pointer to child object (or null if the child does not exist)
BROBase* BROBase::findChild(std::string name)
{
    upperCase(name);
    enterObjectCriticalSection();

    BROBase* ret = 0;

    std::map<std::string,BROBase*>::iterator iter = children.find(name);
    if ( iter!=children.end() ) ret = iter->second;

    leaveObjectCriticalSection();
    return ret;
}


// Sets the list of children of the object in the stem (specified as the argument)
// set the ret value to the number of children found
int BROBase::staticListChildren(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - No stem name specified or too many arguments");
        return 1;
    }

    else if (!BRORexxFunction::isValidStem(args[0]))
    {
        setError(2, "Invalid Arguments - Invalid stem name specified");
        return 1;
    }

    else
    {
        std::vector<BROBase*> childs;
        std::vector<BROBase*>::iterator i;
        std::vector<std::string> names;
        object->listChildren(childs);
        char temp[10];
        for (i=childs.begin(); i!=childs.end(); i++)
        {
            std::string nm;
            (*i)->getName(nm);
            names.push_back(nm);
        }

        BRORexxFunction::SetRexxVariable(args[0], names); // Set rexx variables

        // Return value is set to the number of children found
        sprintf(temp,"%d",(int)names.size());
//    itoa(names.size(), temp, 10);
        ret=temp;
    }
    return 0;
}


// Returns a vector of children BROBases and returns the number found
int BROBase::listChildren(std::vector<BROBase*> &childs)
{
    std::map<std::string, BROBase*>::iterator i;
    childs.clear();

    enterObjectCriticalSection();

    for (i=children.begin(); i!=children.end(); i++)
        childs.push_back((*i).second);

    leaveObjectCriticalSection();

    return childs.size();
}


// Returns the full name of a root element (or blank if the root does not exist)
// Not very useful - although it does allows you to check is a root element exists
int BROBase::staticFindRoot(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - should be OBJECT");
        return 1;
    }
    else
    {
        BROBase* c=object->findRoot(args[0]);
        if (c) c->getFullName(ret);
        else ret="";
    };
    return 0;
}


// Returns the pointer to root object (or null if the root does not exist)
BROBase* BROBase::findRoot(std::string name)
{
    upperCase(name);
    enterStaticCriticalSection();

    BROBase* ret = 0;
    std::map<std::string,BROBase*>::iterator iter = roots.find(name);
    if ( iter!=roots.end() ) ret = iter->second;

    leaveStaticCriticalSection();
    return ret;
};


// Sets the list of roots in the stem (specified as the argument)
// set the ret value to the number of roots found
int BROBase::staticListRoot(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Arguments - No stem name specified or too many arguments");
        return 1;
    }

    else if (!BRORexxFunction::isValidStem(args[0]))
    {
        setError(2, "Invalid Arguments - Invalid stem name specified");
        return 1;
    }

    else
    {
        std::vector<BROBase*> rts;
        std::vector<BROBase*>::iterator i;
        std::vector<std::string> names;
        object->listRoot(rts);
        char temp[10];
        for (i=rts.begin(); i!=rts.end(); i++)
        {
            std::string nm;
            (*i)->getName(nm);
            names.push_back(nm);
        }
        std::string stem = args[0];
        BRORexxFunction::SetRexxVariable(stem, names);

        // Return value is set to the number of children found
        sprintf(temp,"%d",(int)names.size());
//    itoa(names.size(), temp, 10);
        ret=temp;
    }
    return 0;
};


// Returns a vector of root BROBases and returns the number found
int BROBase::listRoot(std::vector<BROBase*> &rts)
{
    std::map<std::string, BROBase*>::iterator i;
    rts.clear();

    enterStaticCriticalSection();

    for (i=roots.begin(); i!=roots.end(); i++)
        rts.push_back((*i).second);

    leaveStaticCriticalSection();

    return rts.size();
};


// Returns the object type in ret
int BROBase::staticGetType(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        setError(1, "Invalid Arguments - should be no arguments");
        return 1;
    }
    else
    {
        object->getType(ret);
        return 0;
    }
};


// Sets tp to the object type
void BROBase::getType(std::string &tp)
{
    enterObjectCriticalSection();
    tp=type;
    leaveObjectCriticalSection();
};


// Sets the external rexx function name for the callback
// Sets ret to 0 for OK -1 for error
int BROBase::staticSetExternalRexx(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=2)
    {
        ret="1";
    }
    else
    {
        object->setExternalRexx(args[0], args[1]);
        ret="0";
    }
    return 0;
};


// Sets the external rexx callback function.
// Also clear the inline rexx information if the rexxxfunction is not blank
void BROBase::setExternalRexx(std::string function, std::string rexxfunctionname)
{
    LOG("BROBase::setExternalRexx(setExternalRexx(function, rexxfunctionname))");
    enterObjectCriticalSection();
    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    if (!func)
    {
        func=new BRORexxFunction(this, function);
        RexxFunctions[function] = func;
    }
    func->setExternalRexx(rexxfunctionname);
    leaveObjectCriticalSection();
};


// Sets the rexx callback function code
// Sets ret to 0 for OK -1 for error
int BROBase::staticSetRexx(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=2)
    {
        setError(1, "Invalid Arguments (should be: function name, rexx code)");
        return 1;
    }
    object->setRexx(args[0], args[1]);

    setError(0, "");
    ret="0";
    return 0;
};


// Rexx Function to see if a rexx function exists
int BROBase::staticRexxFuncExists(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        ret="ERROR 1 - No function name specified or too many arguments";
    }
    else
    {
        if (object->rexxFuncExists(args[0])) ret="1";
        else ret="0";
    }
    return 0;
};



bool BROBase::rexxFuncExists(std::string function)
{
    bool ret;
    enterObjectCriticalSection();

    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    if (func) ret=true;
    else ret=false;
    leaveObjectCriticalSection();
    return ret;
};


// Sets the rexx inline callback information.
// If rexxfunction is blank then it clears the structure and releases memeory
// It also clears the externelrexx function value (if rexxfunction is not blank).
void BROBase::setRexx(std::string function, std::string rexxcode)
{
    LOG("BROBase::setRexx(function, rexxcode)");
    upperCase(function);
    enterObjectCriticalSection();

    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    if (!func)
    {
        func=new BRORexxFunction(this, function);
        RexxFunctions[function] = func;
    }
    func->setRexx(rexxcode);
    leaveObjectCriticalSection();
};


// Sets the rexx inline callback information from a static char* (i.e. code is NOT copied)
// If rexxfunction is null then it clears the structure and releases memeory
// It also clears the externelrexx function value (if rexxfunction is not NULL).
void BROBase::setRexxWithStatic(std::string function, const char* rexxcode)
{
    LOG("BROBase::setRexxWithStatic(function, rexxcode)");
    upperCase(function);
    enterObjectCriticalSection();
    BRORexxFunction* func=0;
    std::map<std::string,BRORexxFunction*>::iterator iter = RexxFunctions.find(function);
    if ( iter!=RexxFunctions.end() ) func = iter->second;

    if (!func)
    {
        func=new BRORexxFunction(this, function);
        RexxFunctions[function] = func;
    }

    func->setRexxWithStatic(rexxcode);
    leaveObjectCriticalSection();
}


// Returns the name of the object in ret
int BROBase::staticGetName(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        setError(1, "Invalid Arguments - should be no arguments");
        return 1;
    }
    else
    {
        object->getName(ret);
        return 0;
    }
}


void BROBase::getName(std::string &nm)
{
    enterObjectCriticalSection();
    nm = name;
    leaveObjectCriticalSection();
}


// Returns the full name (path and name) of the exe being run (if applicable - or blank)
int BROBase::staticGetExeName(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        setError(1, "Invalid Arguments - should be no arguments");
        return 1;
    }
    else
    {
        object->getExeName(ret);
        return 0;
    }
}


void BROBase::getExeName(std::string &nm)
{
    nm = exeName;
}


// Returns the full name of the object in ret
int BROBase::staticGetFullName(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=0)
    {
        setError(1, "Invalid Arguments - should be no arguments");
        return 1;
    }
    else
    {
        object->getFullName(ret);
        return 0;
    }
}


void BROBase::getFullName(std::string &nm)
{
    BROBase* par = getParent();
    if (par)
    {
        par->getFullName(nm);
        std::string myname;
        getName(myname);
        nm = nm + "." + myname;
    }
    else
    {
        getName(nm);
        // nm = "." + nm;
    };
}


// Rexx Function to add a global variable
int BROBase::staticAddGlobalVar(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
   if (args.size()!=1)
    {
        setError(1, "No global name specified or too many arguments");
        return 1;
    }
    else
    {
        if (!object->addGlobalVar(args[0]))
        {
            setError(2, "Invalid rexx variable name: " + args[0]);
            return 2;
        }
    }
    setError(0, "");
    ret="0";
    return 0;
};


// Function to add a global variable
BROVar* BROBase::addGlobalVar(std::string& _name)
{
    std::string nm = _name;
    upperCase(nm);
    enterObjectCriticalSection();
    if (globalVars.count(nm)==0)
    {
        if (!BRORexxFunction::isValidVar(nm))
        {
            leaveObjectCriticalSection();
            return 0;
        }
        globalVars[nm] = new BROVar(nm);
    }

    BROVar* ret = 0;
    std::map<std::string,BROVar*>::iterator iter = globalVars.find(nm);
    if ( iter!=globalVars.end() ) ret = iter->second;

    leaveObjectCriticalSection();
    return ret;
};


// Rexx Function to add a global stem array
int BROBase::staticAddGlobalStem(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "No global name specified or too many arguments");
        return 1;
    }
    else
    {
        if (!object->addGlobalStem(args[0]))
        {
            setError(2, "Invalid rexx stem name: " + args[0]);
            return 2;
        }
    }
    setError(0, "");
    ret="0";
    return 0;
};


// Function to add a global stem array
BROStem* BROBase::addGlobalStem(std::string& _name)
{
    std::string nm = _name;
    upperCase(nm);
    enterObjectCriticalSection();
    if (globalStems.count(nm)==0)
    {
        if (!BRORexxFunction::isValidStem(nm))
        {
            leaveObjectCriticalSection();
            return 0;
        }
        globalStems[nm] = new BROStem(nm);
    }

    BROStem* ret = 0;
    std::map<std::string,BROStem*>::iterator iter = globalStems.find(nm);
    if ( iter!=globalStems.end() ) ret = iter->second;

    leaveObjectCriticalSection();
    return ret;
}


void BROBase::setError(int rc, std::string text)
{
    std::string rcode;
    std::stringstream tmp;
    tmp << rc;
    rcode = tmp.str();
    BRORexxFunction::SetRexxVariable((std::string)"RXORC", rcode);
    BRORexxFunction::SetRexxVariable((std::string)"RXOERRORMSG", text);
}


// Rexx Function to add a global set of filtered variables
int BROBase::staticAddGlobalFilter(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "No global name specified or too many arguments");
        return 1;
    }
    else
    {
        if (!object->addGlobalFilter(args[0]))
        {
            setError(2, "Invalid rexx filter name: " + args[0]);
            return 2;
        }
    }
    setError(0, "");
    ret="0";
    return 0;
}


// Function to add a global set of filtered variables
BROFilter* BROBase::addGlobalFilter(std::string& _name)
{
    std::string nm = _name;
    upperCase(nm);
    BROFilter* ret = 0;
    enterObjectCriticalSection();
    if (globalFilters.count(nm)==0)
    {
        if (!BRORexxFunction::isValidFilter(nm))
        {
            leaveObjectCriticalSection();
            return 0;
        }
        globalFilters[nm] = new BROFilter(nm);
    }

    std::map<std::string,BROFilter*>::iterator iter = globalFilters.find(nm);
    if ( iter!=globalFilters.end() ) ret = iter->second;

    leaveObjectCriticalSection();
    return ret;
}


int BROBase::staticIsValidVar(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "No name specified or too many arguments");
        return 1;
    }
    else if (BRORexxFunction::isValidVar(args[0])) ret="1";
    else ret="0";
    return 0;
};


int BROBase::staticIsValidStem(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "No name specified or too many arguments");
        return 1;
    }
    else if (BRORexxFunction::isValidStem(args[0])) ret="1";
    else ret="0";
    return 0;
};


int BROBase::staticIsValidFilter(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "No name specified or too many arguments");
        return 1;
    }
    else if (BRORexxFunction::isValidFilter(args[0])) ret="1";
    else ret="0";
    return 0;
};


int BROBase::staticLoadVar(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::staticLoadVar");
    if (args.size()!=1) return 1; // Invalid arguments
    std::string nm=args[0];
    upperCase(nm);
    LOG("BROBase::staticLoadVar ... nm=" + nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastPart(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj) return 3; // object not found

    obj->enterObjectCriticalSection();

    BROVar* var = 0;
    std::map<std::string,BROVar*>::iterator iter = (obj->globalVars).find(varname);
    if ( iter!=(obj->globalVars).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        return 2; // variable not found
    }

    LOG("BROBase::staticLoadVar ... var found");
    int r = BRORexxFunction::SetRexxVariable(var);
    obj->leaveObjectCriticalSection();
    return r;
};


int BROBase::staticSaveVar(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1) return 1; // Error
    std::string nm=args[0];
    upperCase(nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastPart(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj) return 3; // object not found

    obj->enterObjectCriticalSection();

    BROVar* var = 0;
    std::map<std::string,BROVar*>::iterator iter = (obj->globalVars).find(varname);
    if ( iter!=(obj->globalVars).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        return 2;
    }

    int r = BRORexxFunction::GetRexxVariable(var);
    obj->leaveObjectCriticalSection();

    return r;
};


int BROBase::staticLoadStem(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1) return 1; // Error
    std::string nm=args[0];
    upperCase(nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastStem(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj) return 3; // object not found

    obj->enterObjectCriticalSection();

    BROStem* var = 0;
    std::map<std::string,BROStem*>::iterator iter = (obj->globalStems).find(varname);
    if ( iter!=(obj->globalStems).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        return 2;
    }
    int r = BRORexxFunction::SetRexxVariable(var);

    obj->leaveObjectCriticalSection();

    return r;
};


int BROBase::staticSaveStem(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1) return 1; // Error
    std::string nm=args[0];
    upperCase(nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastStem(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj) return 3; // object not found

    obj->enterObjectCriticalSection();

    BROStem* var = 0;
    std::map<std::string,BROStem*>::iterator iter = (obj->globalStems).find(varname);
    if ( iter!=(obj->globalStems).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        return 2;
    }
    int r = BRORexxFunction::GetRexxVariable(var);
    obj->leaveObjectCriticalSection();

    return r;
};


int BROBase::staticLoadFilter(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    LOG("BROBase::staticLoadFilter");
    if (args.size()!=1)
    {
        setError(1, "Invalid Argument - Should be: filter");
        return 1;
    }

    std::string nm=args[0];
    upperCase(nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastStem(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj)
    {
        setError(3, "Object not found: " + objname);
        return 1;
    }
    obj->enterObjectCriticalSection();

    LOG("BROBase::staticLoadFilter ... object found");

    BROFilter* var = 0;
    std::map<std::string,BROFilter*>::iterator iter = (obj->globalFilters).find(varname);
    if ( iter!=(obj->globalFilters).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        setError(2, "Global Filter not found: " + varname);
        return 1;
    }

    LOG("BROBase::staticLoadFilter ... filter found");

    int r = BRORexxFunction::SetRexxVariable(var);

    obj->leaveObjectCriticalSection();
    if (r)
    {
        setError(4, "Internal Error in GetRexxVariable rc=" + r);
        return 1;
    }

    ret="0";
    LOG("BROBase::staticLoadFilter ... done");
    return r;
};


int BROBase::staticSaveFilter(BROBase* object, std::vector<std::string> &args, std::string &ret)
{
    if (args.size()!=1)
    {
        setError(1, "Invalid Argument - Should be: filter");
        return 1;
    }

    std::string nm=args[0];
    upperCase(nm);

    std::string varname;
    std::string objname;
    BROBase* obj;
    getLastStem(nm, varname, objname);
    obj = object->findObject(objname);
    if (!obj) // object not found
    {
        setError(3, "Object not found: " + objname);
        return 1;
    }

    obj->enterObjectCriticalSection();

    BROFilter* var = 0;
    std::map<std::string,BROFilter*>::iterator iter = (obj->globalFilters).find(varname);
    if ( iter!=(obj->globalFilters).end() ) var = iter->second;

    if (!var)
    {
        obj->leaveObjectCriticalSection();
        setError(2, "Global Filter not found: " + varname);
        return 1;
    }
    int r = BRORexxFunction::GetRexxVariable(var);
    obj->leaveObjectCriticalSection();
    if (r)
    {
        setError(4, "Internal Error in GetRexxVariable rc=" + r);
        return 1;
    }

    ret="0";
    return r;
};


BROVar* BROBase::addGlobalVar(char* _name)
{
    std::string nm = _name;
    return addGlobalVar(nm);
};


BROStem* BROBase::addGlobalStem(char* _name)
{
    std::string nm = _name;
    return addGlobalStem(nm);
};


BROFilter* BROBase::addGlobalFilter(char* _name)
{
    std::string nm = _name;
    return addGlobalFilter(nm);
};


/********************************************************************/
/* BROBaseFactory Functions                                       */
/********************************************************************/

// The [one and only] factory to make base objects
static BROBaseFactory factory;


// A map of all the factories
std::map<std::string, BROBaseFactory*>* BROBaseFactory::factories;


// Constructor used by sub-classes
BROBaseFactory::BROBaseFactory(const char* tp)
{
//  std::cout << "BROBaseFactory::BROBaseFactory(" << tp << ")" << std::endl << std::flush;
    objectType = tp;
    BROBase::enterStaticCriticalSection();
    if (!factories) factories = new std::map<std::string, BROBaseFactory*>;
    (*factories)[objectType] = this;
    BROBase::leaveStaticCriticalSection();
};


// Constructor
BROBaseFactory::BROBaseFactory()
{
//  std::cout << "BROBaseFactory::BROBaseFactory()" << std::endl << std::flush;
    objectType = "BROBase";
    BROBase::enterStaticCriticalSection();
    if (!factories) factories = new std::map<std::string, BROBaseFactory*>;
    (*factories)[objectType] = this;
    BROBase::leaveStaticCriticalSection();
};


// Destructor
BROBaseFactory::~BROBaseFactory()
{
    BROBase::enterStaticCriticalSection();
    if (!factories) factories = new std::map<std::string, BROBaseFactory*>;
    (*factories).erase(objectType);
    BROBase::leaveStaticCriticalSection();
};


// Function to return a fatory for a object type
BROBaseFactory* BROBaseFactory::findFactory(std::string tp)
{
    BROBase::enterStaticCriticalSection();
    if (!factories) factories = new std::map<std::string, BROBaseFactory*>;
    BROBase::leaveStaticCriticalSection();

    BROBaseFactory* f = 0;
    std::map<std::string,BROBaseFactory*>::iterator iter = factories->find(tp);
    if ( iter!=factories->end() ) f = iter->second;

    return f;
};


// return a new BROBase
BROBase* BROBaseFactory::newObject(BROBase* prnt, std::string nm)
{
    return new BROBase(prnt, nm);
};


// return a new BROBase - the extra arguments are ignored
BROBase* BROBaseFactory::newObject(BROBase* prnt, std::string nm, std::vector<std::string> &args)
{
    return new BROBase(prnt, nm, args);
};


// Function to return a new object
BROBase* BROBaseFactory::newObject(BROBase* prnt, std::string tp, std::string nm)
{
    LOG("BROBaseFactory::newObject(parent, type, name)");
    LOG(" ... type=" + tp);
    BROBaseFactory* factory = findFactory(tp);
    IFELSELOG(factory, " ... Factory Type="+factory->objectType," ... null factory returned");
    if (!factory) throw BROUnknownObjectType(tp);

    BROBase* o;
    try
    {
        if (prnt)
        {
            LOG(" ... calling prnt->beginNewObject()");
            prnt->beginNewObject();
        }
        LOG(" ... calling factory->newObject(prnt, nm)");
        o = factory->newObject(prnt, nm);
        IFELSELOGF(!o, ," ... null object returned",std::string s; o->getType(s), " ... object type: " + s);
    }
    catch (...)
    {
        LOG(" ... Exception caught");
        if (prnt)
        {
            LOG(" ... calling prnt->endNewObject()");
            prnt->endNewObject();
        }
        throw;
    }
    if (prnt)
    {
        LOG(" ... calling prnt->endNewObject()");
        prnt->endNewObject();
    }

    return o;
};


// Function to return a new object with extra arguments
BROBase* BROBaseFactory::newObject(BROBase* prnt, std::string tp, std::string nm,
                                   std::vector<std::string> &args)
{
    LOG("BROBaseFactory::newObject(parent, type, name, args)");
    BROBaseFactory* factory = findFactory(tp);
    IFELSELOG(factory, " ... Factory Type="+factory->objectType," ... null factory returned");
    if (!factory) throw BROUnknownObjectType(tp);

    BROBase* o;
    try
    {
        if (prnt)
        {
            LOG(" ... calling prnt->beginNewObject()");
            prnt->beginNewObject();
        }
        LOG(" ... calling factory->newObject(prnt, nm, args)");
        o = factory->newObject(prnt, nm, args);
        IFELSELOGF(!o, ," ... null object returned",std::string s; o->getType(s), " ... object type: " + s);
    }
    catch (...)
    {
        LOG(" ... Exception caught");
        if (prnt)
        {
            LOG(" ... calling prnt->endNewObject()");
            prnt->endNewObject();
        }
        throw;
    }
    if (prnt)
    {
        LOG(" ... calling prnt->endNewObject()");
        prnt->endNewObject();
    }

    return o;
};


/********************************************************************/
/* Exception Class Functions                                        */
/********************************************************************/

BRODuplicateChildName::BRODuplicateChildName(std::string message) :
        BROException("BRODuplicateChildName: " + message)
{};

BRODuplicateChildName::BRODuplicateChildName() :
        BROException("BRODuplicateChildName")
{};

BRODuplicateRootName::BRODuplicateRootName(std::string message) :
        BROException("BRODuplicateRootName: " + message)
{};

BRODuplicateRootName::BRODuplicateRootName() :
        BROException("BRODuplicateRootName")
{};

BROUnknownObjectType::BROUnknownObjectType(std::string message) :
        BROException("BROUnknownObjectType: " + message)
{};

BROUnknownObjectType::BROUnknownObjectType() :
        BROException("BROUnknownObjectType")
{};

BROInvalidConstructorArguments::BROInvalidConstructorArguments(std::string message) :
        BROException("BROInvalidConstructorArguments: " + message)
{};

BROInvalidConstructorArguments::BROInvalidConstructorArguments() :
        BROException("BROInvalidConstructorArguments")
{};

BROCriticalSectionError::BROCriticalSectionError(std::string message) :
        BROException("BROCriticalSectionError: " + message)
{};

BROCriticalSectionError::BROCriticalSectionError() :
        BROException("BROCriticalSectionError")
{};

BROBaseNoLongerExistsError::BROBaseNoLongerExistsError(std::string message) :
        BROException("BROBaseNoLongerExistsError: " + message)
{};

BROBaseNoLongerExistsError::BROBaseNoLongerExistsError() :
        BROException("BROBaseNoLongerExistsError")
{};
