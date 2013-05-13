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
// File Name   : noconsoletest.cpp
// Description : Shellspawn test harness - to test shellspawn creates a
//             : console window where one is needed
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
#include <fstream>
#include <string>
#include <vector>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

#ifdef FLTK
#include <fltk/run.h>
#include <fltk/Widget.h>
#include <fltk/Window.h>
#include <fltk/Button.h>
#endif

using namespace std;

#include "shellspawn.h"

void Message(char *message);

int main(int argc, char **argv) {

#ifdef UNIX
 // To make sure we are well and truely disconnected from a terminal
 int nfd=open("/dev/null", O_RDWR);

 if (nfd<0)
 {
  perror("Error Opening /dev/null");
  return -1;
 }

 if (dup2(nfd,0)<0)
 {
  perror("Error dup2(nfd,0)");
  return -1;
 }
 if (dup2(nfd,1)<0)
 {
  perror("Error dup2(nfd,1)");
  return -1;
 }
 if (dup2(nfd,2)<0)
 {
  perror("Error dup2(nfd,2)");
 return -1;
 }

 close(nfd);

 int frc=fork();
 if (frc==-1)
 {
  perror("Failure in fork()");
  exit(-1);
 }
 else if (frc>0) exit(0); // Parent - just exit
 // Child process - this is all to make sure setsid works
 if (setsid() == -1)
 {
  perror("Failure in setsid()");
  exit(-1);
 }
#endif

#ifdef GNOME
 gtk_init(NULL, NULL);
#endif

 // No console - so we need to make a log file
 ofstream log;
 log.open("noconsoletest.log");

 /* Hello */
 log << "Test Harness for AV SHELL" << endl << flush;

#ifdef WIN32
 string command("testclient.exe");
#endif

#ifdef UNIX
 string command("testclient");
#endif

 int rc=0;
 string spawnErrorText;
 int spawnErrorCode=0;

 {
  Message("Stdio Test (interactive)");
  log << endl << endl << "Stdio Test (interactive) - \"quit\" closes stdin)" << endl << flush;
  spawnErrorCode = shellspawn(command, NULL, NULL, NULL, stdin,
                              NULL, NULL, NULL, stdout,
                              NULL, NULL, NULL, stderr, rc, spawnErrorText, NULL);
  if (spawnErrorCode == SHELLSPAWN_NOFOUND)
  {
    Message("testclient not found - check current directory");
    log << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl << flush;
    exit(-1);
  }
  if (spawnErrorCode) {
    log << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl << flush;
  }
  log << "RC=" << rc << endl << flush;
 }

 {
  Message("FILE* Test");
  log << endl << endl << "FILE* Test (see input.txt, output.txt and error.txt)" << endl << flush;
  FILE* in=fopen("input.txt","r");
  FILE* out=fopen("output.txt","w");
  FILE* err=fopen("error.txt","w");

  spawnErrorCode = shellspawn(command, NULL, NULL, NULL, in,
                              NULL, NULL, NULL, out,
                              NULL, NULL, NULL, err, rc, spawnErrorText, NULL);
  if (spawnErrorCode) {
    log << "Error Spawning Process. RC=" << spawnErrorCode << ". Error Text=" << spawnErrorText << endl << flush;
  }
  log << "RC=" << rc << endl << flush;

  int rc;

  if (in)
  {
    rc = fclose(in);
    if (rc) log << "Error closing in file" << endl << flush;
  }
  else log << "Warning input.txt does not exist" << endl << flush;

  rc = fwrite("Test Harness added this",23,1,out);
  if (rc!=1) log << "Error writing more to out file - rc was not 1 it was " << rc << endl << flush;
  rc = fclose(out);
  if (rc) log << "Error closing out file" << endl << flush;
  rc = fclose(err);
  if (rc) log << "Error closing err file" << endl << flush;
 }

 log.close();

 return 0;
}

#ifdef FLTK
fltk::Window *window;
void ok_callback(fltk::Widget *, void *) {
  window->hide();
}
#endif

void Message(char *message)
{
#ifdef WIN32
#ifndef FLTK
 MessageBox(NULL, message, "noconsoletest.exe", MB_OK);
#endif
#endif
#ifdef FLTK
 window = new fltk::Window(300, 130, "No Console Test");
 window->begin();
 fltk::Widget *box = new fltk::Widget(20, 40, 260, 50, message);
 fltk::Button ok_button(240,100,50,20,"OK");
 ok_button.callback(ok_callback);
 window->end();
 window->show();
 fltk::run();
#endif
}
