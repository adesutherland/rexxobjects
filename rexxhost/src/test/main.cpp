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
// File Name   : main.cpp
// Description : C++ Test Program for RexxHost
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

using namespace std;

int main()
{
    HROHost::InitHROHost();

    HROHost::lineOut << "RexxHost Test Harness" << endl;

    HROHost* listener = new HROHost(NULL, "testListener", 3000);

    listener->ConnectionListener();

    delete listener;

    HROHost::DoneWithHROHost();

    return 0;
}
