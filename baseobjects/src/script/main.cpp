// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2009 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : script/main.cpp
// Description : Main Script wrapper to allow a Rexx program to be run as a
// script (in boot loader mode - object BOOT, Function LOADER). This has
// access to the base objects shell and has the possibility of adding functions
// and objects and global variables etc.
// If the REXX function returns a object name that exists
// with a function called MAIN then the BOOT Object is deleted and then
// application.main() is executed. This is designed to allow a Bootstrap REXX
// program to set-up an environment for an application to run.
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

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

int bootstrapMain(string exeName, const char* bootstrap, vector<string> &args);

int main(int argc, char **argv) {

  vector<string> args;
  int rc = 0;

  if (argc<2)
  {
      cerr << "RexxObjects Script Loader" << endl;
      cerr << "Version: 0.1" << endl;
      cerr << "Format: rxoscript rexxfile arg1 arg2 ..." << endl;
      return -1;
  }

  for (int i=2; i<argc; i++)
  {
    args.push_back(argv[i]);
  }

  // Read the rexx bootstrap to memory
  FILE* fp=fopen(argv[1],"rb");
  if (!fp)
  {
      cerr << "File not found: " << argv[1] << endl;
      return -1;
  }

  fseek(fp,0,SEEK_END);
  size_t len=ftell(fp); //get length
  fseek(fp,0,SEEK_SET);
  char *bootstrap=(char *)malloc(len+1);
  size_t readrecs = fread(bootstrap,len,1,fp);
  fclose(fp);
  bootstrap[len]=0;

  if (readrecs != 1)
  {
      cerr << "Error reading file: " << argv[1] << endl;
      perror("Error Details: ");
      rc = -1;
  }
  else rc = bootstrapMain(argv[1], bootstrap, args);

  free(bootstrap);
  return rc;
};
