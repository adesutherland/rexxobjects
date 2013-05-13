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
// File Name   : session.h
// Description : C++ header file for the Session Class
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

#ifndef HRORexx_session_h
#define HRORexx_session_h

#include "object.h"
#include "host.h"

// Functions for external functions (to be called from rexx)
class HROSession;
typedef int(*HROSESSION_FUNCTION)(HROSession* object, std::vector<std::string> &args, std::string &ret);

class HROSessionFactory;


class HROSession : public BROBase
{
    friend class HROSessionFactory;
    friend class HROHost;

private:
    int sessionSocket;
    static bool HROSessionInitted;
    HROSession(BROBase* parent, std::string name, int session);
    void HROSession_setup(int socket);
    static void MainWrapper(BROBase* object, void *param);
    void Main();

protected:
    HROSession(BROBase* parent, std::string name);
    HROSession(BROBase* parent, std::string name, std::vector<std::string> &args);
    void HROSession_kill(bool propagate);

public:
    ~HROSession();
};

// Factory Class for making form objects
class HROSessionFactory : public BROBaseFactory
{
public:
    HROSessionFactory();
    BROBase* newObject(BROBase* parent, std::string name);
    BROBase* newObject(BROBase* parent, std::string name, std::vector<std::string> &args);
};

#endif
