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
// File Name   : host.cpp
// Description : C++ implementation of the Host [Listener] Class
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

#include "host.h"
#include "session.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

bool HROHost::HROHostInitted = false;

// Connection Listener Loop
void HROHost::ConnectionListener()
{
    int sessionSocket;
    struct sockaddr_in address; /* Internet socket address stuct */
    int addressSize=sizeof(struct sockaddr_in);

    /* make a socket */
    listenSocket=socket(AF_INET,SOCK_STREAM,0);

    if (listenSocket < 0)
    {
        errorOut << "Could not make a socket" << endl;
    }

    /* fill address struct */
    address.sin_addr.s_addr=INADDR_ANY;
    address.sin_port=htons(listenPort);
    address.sin_family=AF_INET;

    /* bind to a port */
    if (bind(listenSocket,(struct sockaddr*)&address,sizeof(address)) < 0)
    {
        errorOut << "Could not bind" << endl;
    }

    /* establish listen queue */
    if (listen(listenSocket,5) < 0)
    {
        errorOut << "Could not listen" << endl;
    }

    lineOut << "Waiting for a connection on " << listenPort << endl;

    char buffer[15];
    for (int i=0; i<3;i++)
    {
        /* get the connected socket */
        sessionSocket=accept(listenSocket,(struct sockaddr*)&address,(socklen_t *)&addressSize);
        sprintf(buffer, "Session%d", i);
        new HROSession(this, buffer, sessionSocket);
    }
}


// Setup Host Listener
void HROHost::InitHROHost()
{
    InitObject();
};


// Finished with Host Listener
void HROHost::DoneWithHROHost()
{
    DoneWithObject();
};


// Common Constructor Functionality
void HROHost::HROHost_setup(int port)
{
    if (HROHostInitted == false)
    {
        enterStaticCriticalSection();
        if (HROHostInitted == false)
        {
            HROHostInitted = true;
//      if (HROHOST_FUNCTIONS.empty())
//      {
//        HROHOST_FUNCTIONS["EXAMPLE"] = HROHost::staticExample;
//      }
        }
        leaveStaticCriticalSection();
    }
    type="HROHost";
    listenPort = port;
};


HROHost::HROHost(BROBase* parent, std::string name) : BROBase(parent, name)
{
    HROHost_kill(true);
    throw BROInvalidConstructorArguments("Missing port number to listen on");
};


HROHost::HROHost(BROBase* parent, std::string name, std::vector<std::string> &args) : BROBase(parent, name)
{
    int port;

    if (args.size()!=1)
    {
        HROHost_kill(true);
        throw BROInvalidConstructorArguments("1 Required - port number to listen on");
    }

    port  = atoi(args[0].c_str());

    if (port==0)
    {
        HROHost_kill(true);
        throw BROInvalidConstructorArguments("Invalid port number: " + args[0]);
    }

    HROHost_setup(port);
};


HROHost::HROHost(BROBase* parent, std::string name, int port) : BROBase(parent, name)
{
    HROHost_setup(port);
};


// Common Destructor functionality
void HROHost::HROHost_kill(bool propagate)
{
//  enterObjectCriticalSection();

    /* close socket */
    if (listenSocket >= 0)
    {
        if (close(listenSocket) < 0)
        {
            errorOut << "Could not close listenSocket" << endl;
        }
    }
//  leaveObjectCriticalSection();

    if (propagate) BROBase_kill(propagate);
};

// Destructor
HROHost::~HROHost()
{
    HROHost_kill(false); // No propagation as C++ does this for destructors automatically
};


/********************************************************************/
/* VROLabelFactory Functions                                        */
/********************************************************************/

// The [one and only] factory to make label objects
static HROHostFactory factory;

// Constructor
HROHostFactory::HROHostFactory() : BROBaseFactory("HROHost") { };

// Factory function
BROBase* HROHostFactory::newObject(BROBase* parent, std::string name)
{
    return new HROHost(parent, name);
};

BROBase* HROHostFactory::newObject(BROBase* parent, std::string name, std::vector<std::string> &args)
{
    return new HROHost(parent, name, args);
};
