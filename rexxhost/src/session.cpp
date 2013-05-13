// *************************************************************************
// A B O U T   T H I S   W O R K  -  R E X X H O S T
// *************************************************************************
// Work Name   : RexxHost
// Description : Part of RexxObjects - Allow user to connect to a console
//             : of a RexxObject server via telnet or TN3270
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : session.cpp
// Description : C++ implementation of the Session Class
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

#include "session.h"

using namespace std;

bool HROSession::HROSessionInitted = false;

// Main Session Entry Point
void HROSession::Main()
{
    lineOut << "Session Started" << endl;
    char buffer[100];
    int l;
    strcpy(buffer,"Hello\n");

    write(sessionSocket,buffer,strlen(buffer)+1);

    l = read(sessionSocket,buffer,100);
    buffer[l] = 0;
    lineOut << "Received: " << buffer << endl;
}


void HROSession::MainWrapper(BROBase* object, void *param)
{
    ((HROSession*)object)->Main();

    delete (HROSession*)object;
}


// Common Constructor Functionality
void HROSession::HROSession_setup(int socket)
{
    if (HROSessionInitted == false)
    {
        enterStaticCriticalSection();
        if (HROSessionInitted == false)
        {
            HROSessionInitted = true;
//      if (HROSESSION_FUNCTIONS.empty())
//      {
//        HROSESSION_FUNCTIONS["EXAMPLE"] = HROSession::staticExample;
//      }
        }
        leaveStaticCriticalSection();
    }
    type="HROSession";
    sessionSocket = socket;
    startThread(MainWrapper,NULL);
};


HROSession::HROSession(BROBase* parent, std::string name, int session) : BROBase(parent, name)
{
    HROSession_setup(session);
};


HROSession::HROSession(BROBase* parent, std::string name) : BROBase(parent, name)
{
  HROSession_kill(true);
  throw BROInvalidConstructorArguments("Missing sessionSocket");
};


HROSession::HROSession(BROBase* parent, std::string name, std::vector<std::string> &args) : BROBase(parent, name)
{
    int s;

    if (args.size()!=1)
    {
        HROSession_kill(true);
        throw BROInvalidConstructorArguments("1 Required - session socket");
    }

    s = atoi(args[0].c_str());

    if (s == 0)
    {
        HROSession_kill(true);
        throw BROInvalidConstructorArguments("Invalid session socket: " + args[0]);
    }
    HROSession_setup(s);
};


// Common Destructor functionality
void HROSession::HROSession_kill(bool propagate)
{
//  enterObjectCriticalSection();

    /* close socket */
    if (sessionSocket >= 0)
    {
        if (close(sessionSocket) < 0)
        {
             errorOut << "Could not close socket" << endl;
        }
    }
//  leaveObjectCriticalSection();

  if (propagate) BROBase_kill(propagate);
};

// Destructor
HROSession::~HROSession()
{
  HROSession_kill(false); // No propagation as C++ does this for destructors automatically
};



/********************************************************************/
/* VROLabelFactory Functions                                        */
/********************************************************************/

// The [one and only] factory to make label objects
static HROSessionFactory factory;

// Constructor
HROSessionFactory::HROSessionFactory() : BROBaseFactory("HROSession") { };

// Factory function
BROBase* HROSessionFactory::newObject(BROBase* parent, std::string name)
{
    return new HROSession(parent, name);
};

BROBase* HROSessionFactory::newObject(BROBase* parent, std::string name, std::vector<std::string> &args)
{
    return new HROSession(parent, name, args);
};
