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
// File Name   : shelltest.cpp
// Description : Shellspawn test harness 
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

using namespace std;

#include "shellspawn.h"

void OutHandle1(string data, void* )
{
  cout << "OUT(" << data << ")TUO" << endl;
}

void ErrHandle1(string data, void*)
{
    cout << "ERR(" << data << ")RRE" << endl;
}

int InHandle1(string &data, void*)
{
  data="repeat\nBilly\n";
  return 0;
}

int InHandle2(string &data, void*)
{
  cout << "> ";
  getline(cin, data);
  if (data == "quit") return 1;
  data.append("\n");
  return 0;
}


int main(int argc, char **argv) {

 /* Hello */
 cout << "Test Harness for AV SHELL" << endl;

 string command("testclient");
 //string command("test2.cmd");

 int rc=0;
 string spawnErrorText;
 int spawnErrorCode=0;
 int n;

 {
  cout << "Vector Test" << endl;
  vector<string> in;
  vector<string> out;
  vector<string> err;
  in.push_back("Bob Smith");
  spawnErrorCode = shellspawn(command, &in, NULL, NULL, NULL,
                              &out, NULL, NULL, NULL,
                              &err, NULL, NULL, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
  // Display stdout 
  n=out.size();
  if (n==0) cout << "No Stdout" << endl;
  else for (int i=0; i<n; i++) cout << "Stdout line " << (i+1) << ": " << out[i] << endl;
  // Display stderr 
  n=err.size();
  if (n==0) cout << "No Stderr" << endl;
  else for (int i=0; i<n; i++) cout << "Stderr line " << (i+1) << ": " << err[i] << endl;
 }

 {
  cout << endl << endl << "String Test" << endl;
  string sIn = "Jones Simon\n";
  string sOut;
  string sErr;
  spawnErrorCode = shellspawn(command, NULL, &sIn, NULL, NULL,
                              NULL, &sOut, NULL, NULL,
                              NULL, &sErr, NULL, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
  /* Display stdout */
  cout << "Stdout: " << sOut << endl;
  /* Display stderr */
  cout << "Stderr: " << sErr << endl;
 }

 {
  cout << endl << endl << "Call Back Test 1" << endl;
  spawnErrorCode = shellspawn(command, NULL, NULL, InHandle1, NULL,
                              NULL, NULL, OutHandle1, NULL,
                              NULL, NULL, ErrHandle1, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
 }

 {
  cout << endl << endl << "NULL Test" << endl;
  spawnErrorCode = shellspawn(command, NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
 }

 {
  cout << endl << endl << "Command does not exist test - should give an error message" << endl;
  spawnErrorCode = shellspawn("does_not_exist", NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
 }

 {
  string sOut;
  cout << endl << endl << "Command does not exist test - should work - arg is hello" << endl;
  spawnErrorCode = shellspawn("testclient hello", NULL, NULL, NULL, NULL,
                              NULL, &sOut, NULL, NULL,
                              NULL, NULL, NULL, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
  cout << "Stdout: " << sOut << endl;
 }

 {
  cout << endl << endl << "Call Back Test 2 (interactive - \"quit\" closes stdin)" << endl;
  spawnErrorCode = shellspawn(command, NULL, NULL, InHandle2, NULL,
                              NULL, NULL, OutHandle1, NULL,
                              NULL, NULL, ErrHandle1, NULL, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
 }

 {
  cout << endl << endl << "FILE* Test (see input.txt, output.txt and error.txt)" << endl;
  FILE* in=fopen("input.txt","r");
  FILE* out=fopen("output.txt","w");
  FILE* err=fopen("error.txt","w");

  spawnErrorCode = shellspawn(command, NULL, NULL, NULL, in,
                              NULL, NULL, NULL, out,
                              NULL, NULL, NULL, err, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
  int rc;

  if (in)
  {
    rc = fclose(in);
    if (rc) cout << "Error closing in file" << endl;
  }
  else cout << "Warning input.txt does not exist" << endl;

  rc = fwrite("Test Harness added this",23,1,out);
  if (rc!=1) cout << "Error writing more to out file - rc was not 1 it was " << rc << endl;
  rc = fclose(out);
  if (rc) cout << "Error closing out file" << endl;
  rc = fclose(err);
  if (rc) cout << "Error closing err file" << endl;
 }


 {
  cout << endl << endl << "Stdio Test (interactive) - \"quit\" closes stdin)" << endl;
  spawnErrorCode = shellspawn(command, NULL, NULL, NULL, stdin,
                              NULL, NULL, NULL, stdout,
                              NULL, NULL, NULL, stderr, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    cout << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl;
  }
  cout << "RC=" << rc << endl;
 }


 // Loop for Resource leakage test
 // I.e. set it to 10000 and see what happens ....
 {
  cout << endl << endl << "Loop test - look at taskmanger handles/ps/top etc." << endl;
  int loop = 100000;
  vector<string> in;
  vector<string> out;
  vector<string> err;

  for (n=0; n<loop; n++) in.push_back("repeat");
  in.push_back("Jones Simon");

  cout << "looping " << loop << " times (will take a couple of minutes to complete)" << endl;
  for (n=0; n<loop; n++)
  {
     spawnErrorCode = shellspawn(command, &in, NULL, NULL, NULL,
                             &out, NULL, NULL, NULL,
                             &err, NULL, NULL, NULL, rc, spawnErrorText, NULL);
  }

  cout << "Done. Press ENTER to exit" << endl;
  // Note: this must be the last test - so this app ends after this - for the note below to be valid
  cout << "Note: Watch handles etc. in taskmanger to see if they drop suddenly (by 1000s) indicating a leek" << endl;
  cout.flush();
  cin.get();
 }

 return 0;
}
