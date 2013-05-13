// *************************************************************************
// A B O U T   T H I S   W O R K  -   S H E L L S P A W N
// *************************************************************************
// Work Name   : ShellSpawn
// Description : This provides a simple interface to spawn a process 
//             : with redirected input and output
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E  
// *************************************************************************
// File Name   : testclient.cpp
// Description : Test client for shellspawn 
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

using namespace std;
int main(int argc, char **argv)
{
 /* Hello */
 cout << "Test Client for AVShell" << endl;

 /* Test Args */
 if (argc==0) cout << "No arguments" << endl;
 else for (int i=0; i<argc; i++) cout << "Argument " << i << ": " << argv[i] << endl;

 /* Test stderr */
 cerr << "This is an error message" << endl;

 /* Test stdin */
 string name;
 while (true) {
   cout << "What is your name?" << endl;
   getline(cin, name);
   if (name.compare("repeat") != 0) break;
   cout << "Please repeat that!" << endl;
 }
 cout << "Your name is " << name << endl;

 /* Test stderr */
 cerr << "This is another error message" << endl;

 return 123;
}
