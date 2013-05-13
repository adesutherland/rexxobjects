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
// File Name   : unixshell.cpp
// Description : Linux (and perhaps other POSIX etc.) version of shellspawn
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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <termios.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <iostream>
#include <string>
#include <vector>

#include "shellspawn.h"

// Private structure to allow all the threads to share data etc. and
// make the shellspawn() call re-enterent
typedef struct _shelldata {
  vector<string>* vInput;  // data for input stream
  vector<string>* vOutput; // data for output stream
  vector<string>* vError;  // data for error stream
  string* sInput;          // data for input stream
  string* sOutput;         // data for output stream
  string* sError;          // data for error stream
  INHANDLER fInput;        // callback for input stream
  OUTHANDLER fOutput;      // callback for output stream
  OUTHANDLER fError;       // callback for error stream
  int hInputFile;
  int hOutputFile;
  int hErrorFile;
  int inThreadRC;          // Return code for the different threads
  string inThreadErrorText;
  int outThreadRC;
  string outThreadErrorText;
  int errThreadRC;
  string errThreadErrorText;
  int waitThreadRC;
  string waitThreadErrorText;
  int ChildProcessPID;
  int ChildProcessRC;
  /* Pipes/streams fds */
  int hOutputRead, hOutputWrite;
  int hErrorRead,  hErrorWrite;
  int hInputRead,  hInputWrite;
  /* Identifier for threads */
  pthread_t hInThread,  hOutThread,  hErrThread, hWaitThread;
  /* Thread cmmunication */
  pthread_mutex_t *criticalsection;
  //pthread_mutexattr_t mutexType;
  pthread_cond_t *callbackRequested;
  pthread_cond_t *callbackHandled;
  pthread_mutex_t *callbackRequestedMutex;
  pthread_mutex_t *callbackHandledMutex;
  int callbackType; /* 1=StdIn, 2=StdOut or StdErr, -1 means child process exited */
  OUTHANDLER callbackOutputHandler;      // function for output callbacks
  string* callbackBuffer;
  int callbackRC;
  void* context;
  bool needsCreatedConsole;
  int consoleMaster, consoleSlave;
  int consolePID;
  int proxySend, proxyReceive, proxySendRead, proxyReceiveWrite, proxyPID;
  char* buffer;
  char* file_path;
  char** argv;
} SHELLDATA;

// Private functions
static void* HandleInputThread(void* pThreadParam);
static void* HandleOutputThread(void* pThreadParam);
static void* HandleErrorThread(void* pThreadParam);
static void* WaitForProcessThread(void* pThreadParam);
static void WaitForProcess(SHELLDATA* data);
static void Error(string context, string &errorText);
static void CleanUp(SHELLDATA* data);
static int WriteToStdin(string &line, SHELLDATA* data);
static void HandleOutputToVector(int hRead, vector<string>* vOut, int &error, string &errorText);
static void HandleOutputToString(int hRead, string* sOut, int &error, string &errorText);
static void HandleOutputToCallback(int hRead, OUTHANDLER fOut, int &error, string &errorText, SHELLDATA* data);
static void HandleStdinFromVector(SHELLDATA* data);
static void HandleStdinFromCallback(SHELLDATA* data);
static int HandleCallback(SHELLDATA* data, string &errorText);
static int NeedsNewConsole(FILE* file, SHELLDATA* data);
static int ParseCommand(string command_string, char* &command, char* &file, char** &argv);
static int ProxyWorker(SHELLDATA* data);
static void launchChild(SHELLDATA* data);
static void PressToContinue(SHELLDATA* data);
static int CreateConsole(SHELLDATA* data);
static void FreeConsole(SHELLDATA* data);
static bool ExeFound(char* exe);

int shellspawn(string command,
               vector<string>* vIn,
               string* sIn,
               INHANDLER fIn,
               FILE* pIn,
               vector<string>* vOut,
               string* sOut,
               OUTHANDLER fOut,
               FILE* pOut,
               vector<string>* vErr,
               string* sErr,
               OUTHANDLER fErr,
               FILE* pErr,
               int &rc,
               string &errorText,
               void* context)
{
      // Create data structure - and make sure we make all the members empty
      SHELLDATA data;
      data.inThreadRC=0;
      data.inThreadErrorText.clear();
      data.outThreadRC=0;
      data.outThreadErrorText.clear();
      data.errThreadRC=0;
      data.errThreadErrorText.clear();
      data.waitThreadRC=0;
      data.waitThreadErrorText.clear();
      data.ChildProcessPID = 0;
      data.ChildProcessRC = 0;
      data.hInThread = 0;
      data.hOutThread = 0;
      data.hErrThread = 0;
      data.hWaitThread = 0;
      data.hOutputRead = -1;
      data.hOutputWrite = -1;
      data.hErrorRead = -1;
      data.hErrorWrite = -1;
      data.hInputRead = -1;
      data.hInputWrite = -1;
      data.hInputFile = -1;
      data.hOutputFile = -1;
      data.hErrorFile = -1;
      data.criticalsection = 0;
      data.callbackRequested = NULL;
      data.callbackHandled = NULL;
      data.callbackRequestedMutex = NULL;
      data.callbackHandledMutex = NULL;
      data.callbackType = 0;
      data.callbackOutputHandler = NULL;
      data.callbackBuffer = NULL;
      data.callbackRC = 0;
      data.context = context;
      data.needsCreatedConsole = false;
      data.proxySend = -1;
      data.proxyReceive = -1;
      data.proxySendRead = -1;
      data.proxyReceiveWrite = -1;
      data.proxyPID = 0;
      data.buffer = 0;
      data.file_path = 0;
      data.argv = 0;
      data.consoleMaster = -1;
      data.consoleSlave = -1;
      data.consolePID = 0;

      /* Input/Output vectors */
      data.vInput = vIn;
      data.vOutput = vOut;
      data.vError = vErr;
      data.sInput = sIn;
      data.sOutput = sOut;
      data.sError = sErr;
      data.fInput = fIn;
      data.fOutput = fOut;
      data.fError = fErr;

      // Validate inputs
      if ( (vIn?1:0)+(sIn?1:0)+(fIn?1:0)+(pIn?1:0)>1) {
        errorText = "More than one of vIn, sIn, fIn or pIn specified";
        return SHELLSPAWN_TOOMANYIN;
      }
      if ( (vOut?1:0)+(sOut?1:0)+(fOut?1:0)+(pOut?1:0)>1) {
        errorText = "More than one of vOut, sOut, fOut or pOut specified";
        return SHELLSPAWN_TOOMANYOUT;
      }
      if ( (vErr?1:0)+(sErr?1:0)+(fErr?1:0)+(pErr?1:0)>1) {
        errorText = "More than one of vErr, sErr, fErr or pErr specified";
        return SHELLSPAWN_TOOMANYERR;
      }

      // Clear any output strings
      if (data.vOutput) data.vOutput->clear();
      if (data.vError) data.vError->clear();
      if (data.sOutput) data.sOutput->clear();
      if (data.sError) data.sError->clear();

      // Do we need a console and do we need to create one? - these functions work it out
      if (NeedsNewConsole(pIn, &data)<0)
      {
          Error("Failure U1 in NeedsNewConsole(In) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
      }
      if (NeedsNewConsole(pOut, &data)<0)
      {
          Error("Failure U2 in NeedsNewConsole(Out) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
      }
      if (NeedsNewConsole(pErr, &data)<0)
      {
          Error("Failure U3 in NeedsNewConsole(Err) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
      }

      if (data.needsCreatedConsole) // We need to create a console
      {
        if (CreateConsole(&data))
        {
          Error("Failure U4 in CreateConsole() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Do we need the event handlers i.e. Have we any callbacks ...
      if ( (fIn?1:0)+(fOut?1:0)+(fErr?1:0) > 0 ) {
        data.callbackRequested = new pthread_cond_t;
        if (pthread_cond_init(data.callbackRequested, NULL))
        {
          Error("Failure U5 in pthread_cond_init(callbackRequested) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        data.callbackRequestedMutex = new pthread_mutex_t;
        if (pthread_mutex_init(data.callbackRequestedMutex, NULL))
        {
          Error("Failure U6 in pthread_mutex_init(data.callbackRequestedMutex) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        data.callbackHandled = new pthread_cond_t;
        if (pthread_cond_init(data.callbackHandled, NULL))
        {
          Error("Failure U7 in pthread_cond_init(callbackHandled) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        data.callbackHandledMutex = new pthread_mutex_t;
        if (pthread_mutex_init(data.callbackHandledMutex, NULL))
        {
          Error("Failure U8 in pthread_mutex_init(data.callbackHandledMutex) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.criticalsection = new pthread_mutex_t;
        if (pthread_mutex_init(data.criticalsection, NULL))
        {
          Error("Failure U9 in pthread_mutex_init(data.criticalsection) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Create the output pipe and handles
      if (pOut)
      {
       // We have been given a FILE* stream so we want to make a file descriptor
       if (data.needsCreatedConsole && pOut == stdout) data.hOutputFile = data.consoleSlave; // Redirect to our new console
       else data.hOutputFile = fileno(pOut);
      }
      else
      {
        // We Create a pipe
	int temppipe[2];	// This holds the fd for the input & output of the pipe ([0] for reading, [1] for writing)
	if(pipe(temppipe)){
          Error("Failure U10 in pipe() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
	}
        data.hOutputRead = temppipe[0];
        data.hOutputWrite = temppipe[1];
      }
      // Create the standard error output pipe and handles
      if (pErr)
      {
        // We have been given a FILE* stream so we want to make a file descriptor
        if (data.needsCreatedConsole && pErr == stderr) data.hErrorFile = data.consoleSlave; // Redirect to our new console
	else data.hErrorFile = fileno(pErr);
      }
      else
      {
        // We Create a pipe
	int temppipe[2];	// This holds the fd for the input & output of the pipe ([0] for reading, [1] for writing)
	if(pipe(temppipe)){
          Error("Failure U11 in pipe() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
	}
        data.hErrorRead = temppipe[0];
        data.hErrorWrite = temppipe[1];
      }

      // Create the child input pipe.
      if (pIn)
      {
        // We have been given a FILE* stream so we want to make a file descriptor
        if (data.needsCreatedConsole && pIn == stdin) data.hInputFile = data.consoleSlave; // Redirect to our new console
	else data.hInputFile = fileno(pIn);
      }

      else if (fIn)
      {
        // We have been given a function callback we need to create a Pseudo-Terminal Pair ....
        data.hInputWrite = getpt();
        if (data.hInputWrite == -1)
        {
           Error("Failure U12 in getpt() in shellspawn()", errorText);
           CleanUp(&data);
           return SHELLSPAWN_FAILURE;
        }

        if (grantpt(data.hInputWrite) == -1)
        {
           Error("Failure U13 in grantpt() in shellspawn()", errorText);
           CleanUp(&data);
           return SHELLSPAWN_FAILURE;
        }

        if (unlockpt(data.hInputWrite) == -1)
        {
           Error("Failure U14 in unlockpt() in shellspawn()", errorText);
           CleanUp(&data);
           return SHELLSPAWN_FAILURE;
        }
        // We also need to set up the pipes to communicate to our proxy/pseudo shell
        {
	  int temppipe[2];	// This holds the fd for the input & output of the pipe ([0] for reading, [1] for writing)
	  if(pipe(temppipe)){
            Error("Failure U15 in pipe() in shellspawn()", errorText);
            CleanUp(&data);
            return SHELLSPAWN_FAILURE;
	  }
          data.proxySendRead = temppipe[0];
          data.proxySend = temppipe[1];
        }
        {
	  int temppipe[2];	// This holds the fd for the input & output of the pipe ([0] for reading, [1] for writing)
	  if(pipe(temppipe)){
            Error("Failure U16 in pipe() in shellspawn()", errorText);
            CleanUp(&data);
            return SHELLSPAWN_FAILURE;
	  }
          data.proxyReceive = temppipe[0];
          data.proxyReceiveWrite = temppipe[1];
        }
      }

      else
      {
        // We Create a pipe
	int temppipe[2];	// This holds the fd for the input & output of the pipe ([0] for reading, [1] for writing)
	if(pipe(temppipe)){
          Error("Failure U17 in pipe() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
	}
        data.hInputRead = temppipe[0];
        data.hInputWrite = temppipe[1];
      }



      // Parse the command
      char* base_name;
      bool commandFound = false;
      if (ParseCommand(command, data.buffer, base_name, data.argv)) {
        Error("Failure U18 in ParseCommand() in shellspawn()", errorText);
        CleanUp(&data);
        return SHELLSPAWN_NOFOUND;
      }

      if (ExeFound(base_name)) {
        data.file_path = new char[strlen(base_name) + 1];
        strcpy(data.file_path, base_name);
        commandFound = true;
      }

      else if (base_name[0] != '/') {
        // Get PATH environment variable so we can find the exe
        const char* env = getenv("PATH");
        if (env) data.file_path = new char[strlen(env) + strlen(base_name) + 2]; // Make a buffer big enough
        while (env)
        {
          for (int i=0; data.file_path[i]=*env; i++, env++)
          { 
            if (*env==':')
            {
              data.file_path[i]=0;
              env++;
              break;
            }
          }
          
          strcat(data.file_path, "/");
          strcat(data.file_path, base_name);
      
          if (ExeFound(data.file_path)) {
            commandFound = true;
            break;
          }
        }
      }

      if (!commandFound)
      {
        errorText = "Failure U19 in shellspawn() - Command not found";
        CleanUp(&data);
        return SHELLSPAWN_NOFOUND;
      }


      if (fIn) // We need to create a proxy pseudo shell and launch the child process
      {
        if( (data.proxyPID = fork()) == -1){
          Error("Failure U22 in fork() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        if (data.proxyPID == 0) // Proxy Process
        {
          /* Ignore interactive and job-control signals. */
          signal(SIGINT, SIG_IGN);
          signal(SIGQUIT, SIG_IGN);
          signal(SIGTSTP, SIG_IGN);
          signal(SIGTTIN, SIG_IGN);
          signal(SIGTTOU, SIG_IGN);
          signal(SIGCHLD, SIG_DFL);
          signal(SIGHUP, SIG_IGN);

          char *name; // Termainal Name

          // Close Current Termainal and become leader of a new session
          close(0); // Is this needed?
          if (setsid() == -1)
          {
             perror("Failure U23 in setsid() in shellspawn()");
             exit(-1);
          }

          // Open the slave end of the pseudo terminal - which should become the controlling terminal
          name = ptsname(data.hInputWrite);
          if (name == NULL)
          {
             perror("Failure U24 in pstname() in shellspawn()");
             exit(-1);
          }

          data.hInputRead = open(name, O_RDWR); // Is readonly better/safer?
          if (data.hInputRead == -1)
          {
             perror("Failure u25 in open(slave ppt device) in shellspawn()");
             exit(-1);
          }
          dup2(data.hInputFile,0);

          if (isastream(data.hInputRead)) // For System V derived systems ...
          {
             if (ioctl(data.hInputRead, I_PUSH, "ptem") < 0 || ioctl(data.hInputRead, I_PUSH, "ldterm") < 0)
             {
               perror("Failure U26 in ioctl() in shellspawn()");
               exit(-1);
             }
          }

          data.proxyPID = getpid();

          /* Grab control of the terminal. */
          if (tcsetpgrp(data.hInputRead, data.proxyPID) < 0)
          {
               perror("Failure U27 in tcsetpgrp() in shellspawn()");
               exit(-1);
          }

          // Ensure that terminal echo is switched off
          struct termios orig_termios;
          if (tcgetattr(data.hInputRead, &orig_termios) < 0)
          {
               perror("Failure U28 in tcgetattr() in shellspawn()");
               exit(-1);
          }
          orig_termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
          orig_termios.c_oflag &= ~(ONLCR);
          if (tcsetattr(data.hInputRead, TCSANOW, &orig_termios) < 0)
          {
               perror("Failure U29 in tcsetattr() in shellspawn()");
               exit(-1);
          }

          // Launch Child Process
          if ( (data.ChildProcessPID = fork()) == -1)
          {
            perror("Failure U30 in fork() in shellspawn()");
            exit(-1);
          }
          if (data.ChildProcessPID == 0) // Child Process
          {
            // Make this process its own process group
            setpgrp(); // Note: Ignore error conditions as one or other of these will fail
            launchChild(&data); // Does not exit
            exit(-1); // Just in case
          }

          // Make the child process its own process group - done here to avoid any race
          setpgid(data.ChildProcessPID,data.ChildProcessPID); // Note: Ignore error conditions as one or other of these will fail

          // Close unused handles
          close(data.proxySend); data.proxySend = -1;
          close(data.proxyReceive); data.proxyReceive = -1;
          close(data.hOutputRead); data.hOutputRead = -1;
          close(data.hOutputWrite); data.hOutputWrite = -1;
          close(data.hInputWrite); data.hInputWrite = -1;
          close(data.hErrorRead); data.hErrorRead = -1;
          close(data.hErrorWrite); data.hErrorWrite = -1;

          // Send the child process ID to the parent process
          if (write(data.proxyReceiveWrite,(void*)&(data.ChildProcessPID), sizeof(data.ChildProcessPID)) < 0)
          {
            perror("Failure U31 in write() in shellspawn()");
            exit(-1);
          }

          // Start Main Proxy Loop
          int childrc = ProxyWorker(&data);

          // Finish
          close(data.proxySendRead);
          close(data.proxyReceiveWrite);
          exit(childrc);
        }
        else // Main Process
        {
          close(data.proxySendRead); data.proxySendRead = -1;
          close(data.proxyReceiveWrite); data.proxyReceiveWrite = -1;

          // Wait for the childProcessID (also a sync to show that the proxy process has started and set up the terminal etc.)
          if (read(data.proxyReceive, (void*)&(data.ChildProcessPID), sizeof(data.ChildProcessPID)) != sizeof(data.ChildProcessPID))
          {
            Error("Failure U32 in read(did not get child process id from proxy) in shellspawn()", errorText);
            CleanUp(&data);
            return SHELLSPAWN_FAILURE;
          }
        }
      }

      else // Just Launch the redirected command
      {
        if( (data.ChildProcessPID = fork()) == -1){
            Error("Failure U33 in fork() in shellspawn()", errorText);
            CleanUp(&data);
            return SHELLSPAWN_FAILURE;
        }
        if (data.ChildProcessPID == 0) // Child Process
        {
           launchChild(&data);
        }
      }

      // We're the Parent Process ...

      // Close the child ends of any pipes
      if (data.hOutputFile == -1)
      {
          close(data.hOutputWrite);
          data.hOutputWrite = -1;
      }
      if (data.hInputFile == -1)
      {
        close(data.hInputRead);
        data.hInputRead = -1;
      }
      if (data.hErrorFile == -1)
      {
        close(data.hErrorWrite);
        data.hErrorWrite = -1;
      }

      // If we have callbacks lock the cond mutex before starting the threads - in case they try to signal before we start waiting
      if (data.callbackRequested)
      {
        pthread_mutex_lock(data.callbackRequestedMutex);
      }

      // Launch the thread (if needed) that reads the childs standard output
      if (data.hOutputFile == -1)
      {

        if (pthread_create(&(data.hOutThread), NULL, HandleOutputThread, (void *)&data))
        {
          // Error - try and clean-up
          Error("Failure U34 in pthread_create() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Launch the thread (if needed) that reads the childs error output
      if (data.hErrorFile == -1)
      {
        if (pthread_create(&(data.hErrThread), NULL, HandleErrorThread, (void *)&data))
        {
          // Error - try and clean-up
          Error("Failure U35 in pthread_create() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Launch the thread (if needed) that gets the input and sends it to the child.
      if (data.hInputFile == -1)
      {
        if (pthread_create(&(data.hInThread), NULL, HandleInputThread, (void *)&data))
        {
          // Error - try and clean-up
          Error("Failure U36 in pthread_create() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Handle callback request events (so that all callbacks are done in the
      // main thread - to make things easier for the calling process) while
      // waiting for the child process to exit
      if (data.callbackRequested)
      {
        // Thread which waits for the child process and input output threads to complete
        if (pthread_create(&(data.hWaitThread), NULL, WaitForProcessThread, (void *)&data))
        {
          // Error - try and clean-up
          Error("Failure U37 in pthread_create() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        bool processing = true;
        while (processing)
        {
          // Wait for the events
          pthread_cond_wait(data.callbackRequested, data.callbackRequestedMutex);
          if (data.callbackType == -1) processing = false; // Child process and input/output threads have competed
          else {
             if (HandleCallback(&data, errorText))
             {
                pthread_mutex_unlock(data.callbackRequestedMutex);
                CleanUp(&data);
                return SHELLSPAWN_FAILURE;
             }
          }
        }
        pthread_mutex_unlock(data.callbackRequestedMutex);

        // Wait for hWaitThread to finish
        if (pthread_join(data.hWaitThread,NULL))
        {
          // Error - try and clean-up
          Error("Failure U38 in pthread_join(WaitThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }
      else WaitForProcess(&data); // no callback handlers - so we just wait for the process and input/output threads to exit

      // Handle any waitThread errors
      if (data.waitThreadRC)
      {
         errorText.append(data.waitThreadErrorText);
         CleanUp(&data);
         return SHELLSPAWN_FAILURE;
      }

      // Close pipe handles
      if (data.hOutputRead != -1)
      {
        close(data.hOutputRead);
        data.hOutputRead = -1;
      }
      if (data.hErrorRead != -1)
      {
        close(data.hErrorRead);
        data.hErrorRead = -1;
      }
      if (data.proxySend != -1)
      {
        close(data.proxySend);
        data.proxySend = -1;
      }
      if (data.proxyReceive != -1)
      {
        close(data.proxyReceive);
        data.proxyReceive = -1;
      }
      // Note that hInputWrite closed in HandleInputThread() below

      if (data.callbackRequested)
      {
        pthread_cond_destroy(data.callbackRequested);
        delete data.callbackRequested;
        data.callbackRequested = NULL;

        pthread_mutex_destroy(data.callbackRequestedMutex);
        delete data.callbackRequestedMutex;
        data.callbackRequestedMutex = NULL;
      }

      if (data.callbackHandled)
      {
        pthread_cond_destroy(data.callbackHandled);
        delete data.callbackHandled;
        data.callbackHandled = NULL;

        pthread_mutex_destroy(data.callbackHandledMutex);
        delete data.callbackHandledMutex;
        data.callbackHandledMutex = NULL;
      }

      if (data.criticalsection)
      {
        pthread_mutex_destroy(data.criticalsection);
        delete data.criticalsection;
        data.criticalsection = NULL;
      }

      // Close the console if we created one
      if (data.needsCreatedConsole)
      {
        PressToContinue(&data);
        FreeConsole(&data);
      }

      /* Check for errors set by threads */
      if (data.inThreadRC)
      {
         errorText.append(data.inThreadErrorText);
         return SHELLSPAWN_FAILURE;
      }
      if (data.outThreadRC)
      {
         errorText.append(data.outThreadErrorText);
         return SHELLSPAWN_FAILURE;
      }
      if (data.errThreadRC)
      {
         errorText.append(data.errThreadErrorText);
         return SHELLSPAWN_FAILURE;
      }

      rc = (int)data.ChildProcessRC;

 return SHELLSPAWN_OK;
}


void CleanUp(SHELLDATA* data)
{
  if (data->ChildProcessPID) kill(data->ChildProcessPID,15); // 15=TERM, 9=KILL
  if (data->hInThread) pthread_cancel(data->hInThread);
  if (data->hOutThread) pthread_cancel(data->hOutThread);
  if (data->hErrThread) pthread_cancel(data->hErrThread);
  if (data->hWaitThread) pthread_cancel(data->hWaitThread);

  if (data->hOutputRead != -1) close(data->hOutputRead);
  if (data->hOutputWrite != -1) close(data->hOutputWrite);
  if (data->hErrorRead != -1) close(data->hErrorRead);
  if (data->hErrorWrite != -1) close(data->hErrorWrite);
  if (data->hInputRead != -1) close(data->hInputRead);
  if (data->hInputWrite != -1) close(data->hInputWrite);
  if (data->callbackRequested)
  {
    pthread_cond_destroy(data->callbackRequested);
    delete data->callbackRequested;
    data->callbackRequested = NULL;
    pthread_mutex_destroy(data->callbackRequestedMutex);
    delete data->callbackRequestedMutex;
    data->callbackRequestedMutex = NULL;
  }
  if (data->callbackHandled)
  {
    pthread_cond_destroy(data->callbackHandled);
    delete data->callbackHandled;
    data->callbackHandled = NULL;
    pthread_mutex_destroy(data->callbackHandledMutex);
    delete data->callbackHandledMutex;
    data->callbackHandledMutex = NULL;
  }
  if (data->criticalsection)
  {
    pthread_mutex_destroy(data->criticalsection);
    delete data->criticalsection;
    data->criticalsection = NULL;
  }
  data->callbackType = 0;
  data->callbackOutputHandler = NULL;
  data->callbackBuffer = NULL;
  data->callbackRC = 0;
  if (data->needsCreatedConsole) FreeConsole(data);
  if (data->consoleMaster != -1) close(data->consoleMaster);
  if (data->consoleSlave != -1) close(data->consoleSlave);
  if (data->consolePID) kill(data->consolePID,9); // 15=TERM, 9=KILL
  if (data->proxySend != -1) close(data->proxySend);
  if (data->proxyReceive != -1) close(data->proxyReceive);
  if (data->proxySendRead != -1) close(data->proxySendRead);
  if (data->proxyReceiveWrite != -1) close(data->proxyReceiveWrite);
  if (data->proxyPID) kill(data->proxyPID,9); // 15=TERM, 9=KILL
  if (data->buffer) delete[] data->buffer;
  if (data->argv) delete[] data->argv;
  if (data->file_path) delete[] data->file_path;
}

// Routine to see if a FILE* descriptor implies that we need a create a new console
// Creates a console if need be
int NeedsNewConsole(FILE* file, SHELLDATA* data)
{
  struct stat buf;
  dev_t dev, nullDev;

  if (data->needsCreatedConsole) return 0; // Job already done

  if (file == NULL) return 0;    // Null - not relavent console not needed

  if (file == stdin || file == stdout || file == stderr)
  {
    if (fstat(fileno(file), &buf)<0) data->needsCreatedConsole = true; // Error - I guess we need a terminal then ...
    else
    {
     dev = buf.st_dev;
     if (stat("/dev/null", &buf)<0) return -1;
     nullDev = buf.st_dev;
     if (dev==nullDev) data->needsCreatedConsole = true;
    }
  }

  return 0;
};


/* Procedure - running in the main thread - to call the caller's callback handlers */
int HandleCallback(SHELLDATA* data, string &errorText)
{
  INHANDLER inFunc;
  OUTHANDLER outFunc;
  switch (data->callbackType)
  {
    case 1: // Stdin
      inFunc = data->fInput;
      data->callbackRC = inFunc(*(data->callbackBuffer), data->context);
     break;

    case 2: // Stdout or StdErr
      outFunc = data->callbackOutputHandler;
      outFunc(*(data->callbackBuffer), data->context);
      break;

    default:
     // Something bad has happended - this has gone wrong
     errorText="Failure U39 in HandleCallback() in HandleCallback(). Internal Error: Unexpected callbackType";
     return -1;
  }

  // Cleanup
  data->callbackType = 0;
  data->callbackOutputHandler = NULL;
  data->callbackBuffer = NULL;

  // Signal the in, out or err thread that the callback has been handled
  if (pthread_mutex_lock(data->callbackHandledMutex))
  {
    Error("Failure U40 in pthread_mutex_lock(callbackHandledMutex) in HandleCallback()", errorText);
    return -1;
  }

  if (pthread_cond_signal(data->callbackHandled))
  {
    Error("Failure U41 in pthread_cond_signal(callbackHandled) in HandleCallback()", errorText);
    return -1;
  }

  if (pthread_mutex_unlock(data->callbackHandledMutex))
  {
    Error("Failure U42 in pthread_mutex_unlock(callbackHandledMutex) in HandleCallback()", errorText);
    return -1;
  }

  return 0; // Success
}


// Thread Wrapper for the WaitforProcess function
void* WaitForProcessThread(void* pThreadParam)
{
  SHELLDATA* data = (SHELLDATA*)pThreadParam;
  WaitForProcess(data);

  // Fire callbackrequested type -1 to indicate that the child process and all the threads are done
  pthread_mutex_lock(data->callbackRequestedMutex);
  data->callbackType = -1;
  pthread_cond_signal(data->callbackRequested);
  pthread_mutex_unlock(data->callbackRequestedMutex);

  return NULL;
};


// Waits for the child process and all the input/output thread handlers to exit.
void WaitForProcess(SHELLDATA* data)
{
  pid_t w;
  int status;

  // Wait for child process to exit
  int pid;
  if (data->fInput) pid = data->proxyPID; // We get the child exit/status via the proxy status
  else pid = data->ChildProcessPID;

  do {
    w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    if (w == -1)
    {
       data->waitThreadRC = 1;
       Error("Failure U43 in waitpid() in WaitForProcess()", data->waitThreadErrorText);
       return;
    };
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  data->ChildProcessPID = 0;
  data->proxyPID = 0;

  // Get Return Code
  data->ChildProcessRC = WEXITSTATUS(status);

  // Wait for the Output thread to die.
  if (data->hOutThread)
  {
    if (pthread_join(data->hOutThread,NULL))
    {
      // Error - try and clean-up
      data->waitThreadRC = 1;
      Error("Failure U44 in pthread_join(OutThread) in WaitForProcess()", data->waitThreadErrorText);
      return;
    }
  }
  data->hOutThread = 0;

  // Wait for the Error thread to die.
  if (data->hErrThread)
  {
    if (pthread_join(data->hErrThread,NULL))
    {
      // Error - try and clean-up
      data->waitThreadRC = 1;
      Error("Failure U45 in pthread_join(ErrThread) in WaitForProcess()", data->waitThreadErrorText);
      return;
    }
  }
  data->hErrThread = 0;

  // Wait for the input thread to die.
  if (data->hInThread)
  {
    if (pthread_join(data->hInThread,NULL))
    {
      // Error - try and clean-up
      data->waitThreadRC = 1;
      Error("Failure U46 in pthread_join(InThread) in WaitForProcess()", data->waitThreadErrorText);
      return;
    }
  }
  data->hInThread = 0;
};


/* Thread process to handle standard output */
void* HandleOutputThread(void* lpvThreadParam)
{
  SHELLDATA* data = (SHELLDATA*)lpvThreadParam;
  if (data->vOutput)
         HandleOutputToVector(data->hOutputRead,
                              data->vOutput,
                              data->outThreadRC,
                              data->outThreadErrorText);

  else if (data->sOutput)
         HandleOutputToString(data->hOutputRead,
                              data->sOutput,
                              data->outThreadRC,
                              data->outThreadErrorText);

  else if (data->fOutput)
         HandleOutputToCallback(data->hOutputRead,
                              data->fOutput,
                              data->outThreadRC,
                              data->outThreadErrorText,
                              data);
  else {
         // Read and discard output
         HandleOutputToString(data->hOutputRead,
                              NULL,
                              data->outThreadRC,
                              data->outThreadErrorText);
  }
  return NULL;
};

/* Thread process to handle standard error */
void* HandleErrorThread(void* lpvThreadParam)
{
  SHELLDATA* data = (SHELLDATA*)lpvThreadParam;
  if (data->vError)
         HandleOutputToVector(data->hErrorRead,
                              data->vError,
                              data->errThreadRC,
                              data->errThreadErrorText);

  else if (data->sError)
         HandleOutputToString(data->hErrorRead,
                              data->sError,
                              data->errThreadRC,
                              data->errThreadErrorText);

  else if (data->fError)
         HandleOutputToCallback(data->hErrorRead,
                              data->fError,
                              data->errThreadRC,
                              data->errThreadErrorText,
                              data);

  else {
         // Read and discard output
         HandleOutputToString(data->hErrorRead,
                              NULL,
                              data->errThreadRC,
                              data->errThreadErrorText);
  }
  return NULL;
};

/* Function to handle output to a vector of strings */
void HandleOutputToVector(int hRead, vector<string>* vOut, int &error, string &errorText)
{
      char lpBuffer[256+1]; // Add one for a trailing null if needed
      int nBytesRead;
      string *buffer=0;
      int start;
      bool reading = true;

      while(reading)
      {
         nBytesRead = read(hRead, lpBuffer, 256);
         if (nBytesRead == 0) reading = false;
         else if (nBytesRead == -1)
         {
           error = 1;
           Error("Failure U47 in read() in HandleOutputToVector()", errorText);
           return;
         }
         start=0;
         for (int i=0; i<(int)nBytesRead; i++) {
           if (lpBuffer[i] == '\n') {
              lpBuffer[i]=0;
              if (!buffer) buffer = new string;
              buffer->append(lpBuffer+start);
              vOut->push_back((*buffer));
              delete buffer;
              buffer=0;
              start=i+1;
           }
         }
         if (start<(int)nBytesRead) {
           lpBuffer[nBytesRead]=0;
           if (!buffer) buffer = new string;
           buffer->append(lpBuffer+start);
         }
      }

      /* Add the last line if need be */
      if (buffer) {
        vOut->push_back((*buffer));
        delete buffer;
      }

      return;
}

/* Function to handle output to a strings */
void HandleOutputToString(int hRead, string* sOut, int &error, string &errorText)
{
      char lpBuffer[256+1]; // Add one for a trailing null if needed
      int nBytesRead;
      bool reading = true;

      while(reading)
      {
         nBytesRead = read(hRead, lpBuffer, 256);
         if (nBytesRead == 0) reading = false;
         else if (nBytesRead == -1)
         {
              error = 1;
              Error("Failure U48 in read() in HandleOutputToString()", errorText);
              return;
         }
         if (sOut) { // if sOut is null discard output
            lpBuffer[nBytesRead]=0;
            sOut->append(lpBuffer);
         }
      }

      return;
}

/* Function to handle output to a callback */
void HandleOutputToCallback(int hRead, OUTHANDLER fOut, int &error,
                            string &errorText, SHELLDATA* data)
{
      char lpBuffer[256+1]; // Add one for a trailing null if needed
      int nBytesRead;
      bool reading = true;
      string buffer;

      while(reading)
      {
         nBytesRead = read(hRead, lpBuffer, 256);
         if (nBytesRead == 0) reading = false;
         else if (nBytesRead == -1)
         {
              error = 1;
              Error("Failure U49 in read() in HandleOutputToCallback()", errorText);
              return;
         }

         if (nBytesRead)
         {
           lpBuffer[nBytesRead]=0;
           buffer.append(lpBuffer);
           // Critical section is used to ensure that one callback is called at a time
           if (pthread_mutex_lock(data->criticalsection))
           {
             error = 1;
             Error("Failure U50 in pthread_mutex_lock(criticalsection) in HandleOutputToCallback()", errorText);
             return;
           }

           // OK we need to signal the main thread to do the callback for us so that all
           // callbacks run on the main thread - this helps the calling system
           // Set up the common data
           data->callbackType = 2; // Output
           data->callbackOutputHandler = fOut;
           data->callbackBuffer = &buffer;

           // Signal the main thread
           if (pthread_mutex_lock(data->callbackRequestedMutex))
           {
             error = 1;
             Error("Failure U51 in pthread_mutex_lock(callbackRequestedMutex) in HandleOutputToCallback()", errorText);
             return;
           }
           if (pthread_cond_signal(data->callbackRequested))
           {
             error = 1;
             Error("Failure U52 in pthread_cond_signal(callbackRequested) in HandleOutputToCallback()", errorText);
             return;
           }

           // Wait for the main thread to have done the work
           if (pthread_mutex_lock(data->callbackHandledMutex)) // Lock the callback before unlocking the request
           {
             error = 1;
             Error("Failure U53 in pthread_mutex_lock(callbackHandledMutex) in HandleOutputToCallback()", errorText);
             return;
           }
           if (pthread_mutex_unlock(data->callbackRequestedMutex))
           {
             error = 1;
             Error("Failure U54 in pthread_mutex_unlock(callbackRequestedMutex) in HandleOutputToCallback()", errorText);
             return;
           }
           if (pthread_cond_wait(data->callbackHandled, data->callbackHandledMutex))
           {
             error = 1;
             Error("Failure U55 in pthread_cond_wait(callbackHandled) in HandleOutputToCallback()", errorText);
             return;
           }
           if (pthread_mutex_unlock(data->callbackHandledMutex))
           {
             error = 1;
             Error("Failure U56 in pthread_mutex_unlock(callbackHandledMutex) in HandleOutputToCallback()", errorText);
             return;
           }

           if (pthread_mutex_unlock(data->criticalsection))
           {
             error = 1;
             Error("Failure U57 in pthread_mutex_unlock(criticalsection) in HandleOutputToCallback()", errorText);
             return;
           }

           buffer.clear();
         }
      }

      return;
}


/* Thread process to handle standard input */
void* HandleInputThread(void* lpvThreadParam)
{
  SHELLDATA* data = (SHELLDATA*)lpvThreadParam;
  if (data->vInput)
         HandleStdinFromVector(data);

  else if (data->sInput)
         WriteToStdin(*(data->sInput), data);

  else if (data->fInput)
         HandleStdinFromCallback(data);

  // else  - Nothing to do ... just close the handle ... i.e. as below
  close(data->hInputWrite);
  data->hInputWrite = -1;
  return NULL;
}

void HandleStdinFromVector(SHELLDATA* data)
{
  int n = data->vInput->size();
  for (int i=0; i<n; i++)
  {
    string buffer = (*(data->vInput))[i] + "\n";
    if (WriteToStdin(buffer, data)) break;
  }
}

void HandleStdinFromCallback(SHELLDATA* data)
{
  char CommBuffer[1];
  int rc;
  string line;

  do
  {
    // Wait for the proxy to tell us that input is needed
    rc = read(data->proxyReceive, (void*)CommBuffer, 1);
    if (rc == -1)
    {
      data->inThreadRC = 1;
      Error("Failure U58 in read(proxyReceive) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }
    if (rc == 0) return; // Proxy has exited - we're done

    // Critical section is used to ensure that one callback is called at a time
    // I.e. only one callback from in, out or err at a  time so that this
    // looks single threaded for the caller of shellspawn()
    if (pthread_mutex_lock(data->criticalsection))
    {
      data->inThreadRC = 1;
      Error("Failure U59 in pthread_mutex_lock(criticalsection) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    // OK we need to signal the main thread to do the callback for us so that all
    if (pthread_mutex_lock(data->callbackRequestedMutex))
    {
      data->inThreadRC = 1;
      Error("Failure U60 in pthread_mutex_lock(callbackRequestedMutex) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }
    // callbacks run on the main thread - this helps the calling system
    // Set up the comon data
    data->callbackType = 1; // Input
    data->callbackBuffer = &line;
    if (pthread_cond_signal(data->callbackRequested))
    {
      data->inThreadRC = 1;
      Error("Failure U61 in pthread_cond_signal(callbackRequested) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    // Wait for the main thread to have done the work
    if (pthread_mutex_lock(data->callbackHandledMutex)) // Lock the callback before unlocking the request
    {
      data->inThreadRC = 1;
      Error("Failure U62 in pthread_mutex_lock(callbackHandledMutex) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    if (pthread_mutex_unlock(data->callbackRequestedMutex))
    {
      data->inThreadRC = 1;
      Error("Failure U63 in pthread_mutex_unlock(callbackRequestedMutex) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    if (pthread_cond_wait(data->callbackHandled, data->callbackHandledMutex))
    {
      data->inThreadRC = 1;
      Error("Failure U64 in pthread_cond_wait(callbackHandled) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    // Get the return code
    int callbackrc=data->callbackRC;

    // Cleanup
    data->callbackRC=0;

    if (pthread_mutex_unlock(data->callbackHandledMutex))
    {
      data->inThreadRC = 1;
      Error("Failure U65 in pthread_mutex_unlock(callbackHandledMutex) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    if (pthread_mutex_unlock(data->criticalsection))
    {
      data->inThreadRC = 1;
      Error("Failure U66 in pthread_mutex_unlock(criticalsection) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }

    if ( callbackrc )  // We were asked to kill the input stream - we just need to return
    {
      // First Tell the proxy that we have closed the input
      CommBuffer[0]='C'; // C=Closed Terminal
      if (write(data->proxySend,(void*)CommBuffer, 1) < 0)
      {
        data->inThreadRC = 1;
        Error("Failure U67 in write(proxySend) in HandleStdinFromCallback()", data->inThreadErrorText);
        return;
      }
      return;
    }

    // Write the line - exit on any error
    if (WriteToStdin(line, data))
    {
      CommBuffer[0]='E'; // E=Error
      write(data->proxySend,(void*)CommBuffer, 1);
      return;
    }

    // Tell the proxy that we have sent the input
    CommBuffer[0]='X';
    if (write(data->proxySend,(void*)CommBuffer, 1) < 0)
    {
      data->inThreadRC = 1;
      Error("Failure U68 in write(proxySend) in HandleStdinFromCallback()", data->inThreadErrorText);
      return;
    }
  } while (1);
}


int WriteToStdin(string &line, SHELLDATA* data)
{
  int nTotalWrote=0;
  char* buff = (char*)line.c_str();
  int nBytes = line.length();
  int nBytesWrote=0;
  sigset_t signal_mask;

  // Use pthread_sigmask to block the SIG-PIPE (in case we write to the pipe after it was closed by the child process)
  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL)) {
    data->inThreadRC = 1;
    Error("Failure U69 in pthread_sigmask() in WriteToStdin()", data->inThreadErrorText);
    return -1;
  }

  while (nTotalWrote<nBytes)
  {
    nBytesWrote = write(data->hInputWrite, (void*)(buff+nTotalWrote), (nBytes-nTotalWrote));

    if (nBytesWrote == -1)
    {
      if (errno == EPIPE) {
         // Pipe was closed, a normal exit path - the child exitted before processing all input
         return 1;
      }
      else {
        data->inThreadRC = 1;
        Error("Failure U70 in write() in WriteToStdin()", data->inThreadErrorText);
        return -1;
      }
    }
    nTotalWrote += nBytesWrote;
  }
  return 0;
}

void Error(string context, string &errorText)
{
  char sRC[10];
  sprintf(sRC, "%d", errno);
  errorText = context + ". Unix details: RC=" + sRC + " Text=" + (char*)strerror(errno);
}

/* Parse the command to get the arguments */
int ParseCommand(string command_string, char* &command, char* &file, char** &argv)
{
  int l=0;
  int args = 1;
  int a;
  int arg_start;

  command = new char[command_string.length() + 1];
  if(command == NULL) {
    command = 0;
    file = 0;
    argv = 0;
    return -1;
  }
  strcpy(command, command_string.c_str());

  // Skip Leading Spaces
  for (; command[l]; l++) if (command[l]!=' ') break;

  // Program bin/exe name
  file=command+l;
  for (; command[l]; l++) if (command[l]==' ') break;
  if (command[l]!=0) {
    command[l] = 0;
    l++;
  }

  // Is there any command at all
  if (!file[0]) {
   delete[] command;
   command = 0;
   file = 0;
   argv = 0;
   return -1;
  }

  // Skip Trailing Spaces
  for (; command[l]; l++) if (command[l]!=' ') break;

  if (command[l] != 0) { // There are some arguments
    arg_start = l;

    // Count Arguments
    while (command[l]) {
      switch (command[l])
      {
        case '"':
         // Read to the end of the string
         for (l++; command[l]; l++) if (command[l]=='"') {
           l++;
           break;
         }
         args++;
         break;
        case '\'':
         // Read to the end of the string
         for (l++; command[l]; l++) if (command[l]=='\'') {
           l++;
           break;
         }
         args++;
         break;
        default:
         for (l++; command[l]; l++) if (command[l]==' ') {
           l++;
           break;
         }
         args++;
         break;
      }
      // Skip Trailing Spaces
      for (; command[l]; l++) if (command[l]!=' ') break;
    }
   }

   argv = new char*[args+1];
   if(argv == NULL) {
     delete[] command;
     command = 0;
     file = 0;
     argv = 0;
     return -1;
   }
   if( ( argv[0] = strrchr( file, '/' ) ) != NULL )
      argv[0]++;
   else
      argv[0] = file;

   // Null Terminator
   argv[args] = 0;

   // Process Arguments
   if (args>1) {
     a = 1;
     l = arg_start;
     while (command[l]) {
       switch (command[l])
       {
         case '"':
          argv[a] = command+l+1;
          for (l++; command[l]; l++) {
            if (command[l]=='"') {
              command[l]=0;
              l++;
              break;
            }
          }
          a++;
          break;
         case '\'':
          argv[a] = command+l+1;
          for (l++; command[l]; l++) {
            if (command[l]=='\'') {
              command[l]=0;
              l++;
              break;
            }
          }
          a++;
          break;
         default:
          argv[a] = command+l;
          for (l++; command[l]; l++) {
            if (command[l]==' ') {
              command[l]=0;
              l++;
              break;
            }
          }
          a++;
          break;
       }
       // Skip Trailing Spaces
       for (; command[l]; l++) if (command[l]!=' ') break;
     }
   }

  return 0;
}


// Main loop for the proxy/pseudo shell
// This takes care of waiting for signals on the child and moving the child to the forground and background
int ProxyWorker(SHELLDATA* data)
{
  pid_t w;
  int status;
  fd_set set;
  struct timeval timeout;
  char CommBuffer[1];
  // Wait for child process to exit (or to be stopped with SIGTTIN
  do {
    w = waitpid(data->ChildProcessPID, &status, WUNTRACED | WCONTINUED);

    if (w == -1)
    {
       perror("Failure U71 in waitpid() in shellspawn()");
       return -1;
    };

    if (WIFSTOPPED(status))
    {
      if (WSTOPSIG(status) == SIGTTIN)
      {
        /* Initialize the timeout data structure. */
        timeout.tv_sec = 0; // seconds
        timeout.tv_usec = 0;
        /* Initialize the ﬁle descriptor set. */
        FD_ZERO (&set);
        FD_SET (data->hInputRead, &set);
        /* select returns 0 if timeout, 1 if input available, -1 if error. */
        switch (select (1, &set, NULL, NULL, &timeout))
        {
          case 0: // Nothing in buffer to read
            // Kick the main process to send some input
            CommBuffer[0]='X';
            write(data->proxyReceiveWrite,(void*)CommBuffer, 1);

            // Put the job into the foreground
            if (tcsetpgrp(data->hInputRead, data->ChildProcessPID) < 0)
            {
              perror("Failure U72 in tcsetpgrp(ChildProcess) in shellspawn()");
              return -1;
            }

            // Continue it
            if (kill (- data->ChildProcessPID, SIGCONT) < 0)
            {
              perror("Failure U73 in kill(SIGCONT) in shellspawn()");
              return -1;
            }

            // Wait for the main process to confirm that it has put data into the input buffer
            if (read(data->proxySendRead, (void*)CommBuffer, 1) != 1)
            {
              perror("FailureU74 in read(did not get input confirmation) in shellspawn()");
              return -1;
            }
            // Note: Don't bother with waiting 10ms (or whatever) here for the child to complete reading the input as we have waited for the main
            // process to trigger us anyway - and if need be it will be picked up next time round...
            // timeout.tv_sec = 0; // seconds
            //timeout.tv_usec = 10;
            //if (select(0, NULL, NULL, NULL, &timeout) == -1)
            //{
            //  cerr << "Failure 16X in select(waiting 100ms) in shellspawn()" << endl;
            //  return -1;
            //}
            if (CommBuffer[0]=='X') // I.e. Not C (Terminal is being closed) or E (Error)
            {
              // Stop the job
              if (kill (- data->ChildProcessPID, SIGSTOP) < 0)
              {
                perror("Failure U75 in kill(SIGSTOP) in shellspawn()");
                return -1;
              }

              // Put the job into the background
              if (tcsetpgrp(data->hInputRead, data->proxyPID)<0)
              {
                perror("Failure U76 in tcsetpgrp(set proxy to foreground) in shellspawn()");
                return -1;
              };

              // Continue it
              if (kill (- data->ChildProcessPID, SIGCONT) < 0)
              {
                perror("Failure U77 in kill(SIGCONT) in WaitForProcess()");
                return -1;
              }
            }
            break;

          case 1: // Somthing to read from buffer
            // Put the job into the foreground
            if (tcsetpgrp(data->hInputRead, data->ChildProcessPID) < 0)
            {
              perror("Failure U78 in tcsetpgrp(ChildProcess) in shellspawn()");
              return -1;
            }
            // Continue it
            if (kill (- data->ChildProcessPID, SIGCONT) < 0)
            {
             perror("Failure U79 in kill(SIGCONT) in shellspawn()");
             return -1;
            }
            // Wait 50 ms for the child complete reading the input
            timeout.tv_sec = 0; // seconds
            timeout.tv_usec = 50;
            if (select(0, NULL, NULL, NULL, &timeout) == -1)
            {
              perror("Failure U80 in select(waiting 100ms) in shellspawn()");
              return -1;
            }
            // Stop the job
            if (kill (- data->ChildProcessPID, SIGSTOP) < 0)
            {
              perror("Failure U81 in kill(SIGSTOP) in shellspawn()");
              return -1;
            }
            // Put the job into the background
            if (tcsetpgrp(data->hInputRead, data->proxyPID) < 0)
            {
              perror("Failure U82 in tcsetpgrp(Proxy) in shellspawn()");
              return -1;
            }
            // Continue it
            if (kill (- data->ChildProcessPID, SIGCONT) < 0)
            {
              perror("Failure U83 in kill(SIGCONT) in shellspawn()");
              return -1;
            }
            break;

          default:
            perror("Failure U84 in select() in shellspawn()");
            return -1;
        }
      }
    }
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));

  // return the child RC
  return WEXITSTATUS(status);
}


// Launches the child job - never returnes
void launchChild(SHELLDATA* data)
{
  if (data->hOutputFile != -1) {
    dup2(data->hOutputFile,1);
  }
  else {
    dup2(data->hOutputWrite,1);
    close(data->hOutputRead);
  }

  if (data->hInputFile != -1) {
    dup2(data->hInputFile,0);
  }
  else {
    dup2(data->hInputRead,0);
    close(data->hInputWrite);
  }

  if (data->hErrorFile != -1) {
    dup2(data->hErrorFile,2);
  }
  else {
    dup2(data->hErrorWrite,2);
    close(data->hErrorRead);
  }

  /* Set the handling for job control signals back to the default. */
  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);

  // Execute the command
  execv(data->file_path, data->argv);
  perror("Failure U85 execv() Error");
  exit(-1);
};

int CreateConsole(SHELLDATA* data)
{
 char *name; // Termainal Name

 // we need to create a Pseudo-Terminal Pair ....
 data->consoleMaster = getpt();
 if (data->consoleMaster == -1) return -1;
 if (grantpt(data->consoleMaster) == -1) return -1;
 if (unlockpt(data->consoleMaster) == -1) return -1;

 // Open the slave end of the pseudo terminal
 name = ptsname(data->consoleMaster);
 if (name == NULL) return -1;
 data->consoleSlave = open(name, O_RDWR | O_NOCTTY);
 if (data->consoleSlave == -1) return -1;

 if (isastream(data->consoleSlave)) // For System V derived systems ...
 {
   if (ioctl(data->consoleSlave, I_PUSH, "ptem") < 0 || ioctl(data->consoleSlave, I_PUSH, "ldterm") < 0)
   return -1;
 }

 if( (data->consolePID = fork()) == -1) return -1;
 if (data->consolePID == 0) // Child Process
 {
   close(data->consoleSlave);
   char option[10];
   sprintf(option, "-S%c/%d", name[strlen(name)-1], data->consoleMaster);
   if (execlp("xterm", "xterm", option, (char *)0)) perror("execl xterm");
   exit(-1);
 }

 // Read the Window ID that xterm writes for us - we don't use it
 char buf[80];
 read(data->consoleSlave, buf, 80);

  // Clear Screen
 write(data->consoleSlave, "\033[2J\033[H", 7);

 // We don't need the console master
 close(data->consoleMaster);
 data->consoleMaster = -1;
 return 0;
}

void FreeConsole(SHELLDATA* data)
{
  if (data->consoleSlave != -1) close(data->consoleSlave);
  data->consoleSlave = -1;

  if (data->consoleMaster != -1) close(data->consoleMaster);
  data->consoleMaster = -1;

  if (data->consolePID) kill(data->consolePID, 15); // 15=TERM, 9=KILL
  data->consolePID = 0;

  data->needsCreatedConsole = false;
  return;
}

void PressToContinue(SHELLDATA* data)
{
 if (data->consoleSlave > -1)
 {
  char* prompt = "\nCommand Completed - Press ENTER to continue\n";
  char buf[1];
  write(data->consoleSlave, prompt, strlen(prompt));
  read(data->consoleSlave, buf, 1);
 }
};

bool ExeFound(char* exe)
{
  // Stat the command to see if it exists
  struct stat stat_p;
  if ( stat(exe, &stat_p) ) return false;
  if (!S_ISREG(stat_p.st_mode)) return false; // Not a regular file
  if ( !( ((stat_p.st_mode & S_IXUSR) && (stat_p.st_uid==geteuid())) ||
          ((stat_p.st_mode & S_IXGRP) && (stat_p.st_gid==getegid())) ||
           (stat_p.st_mode & S_IXOTH) ) ) return false; // Does not have exec permission

  return true;
}
