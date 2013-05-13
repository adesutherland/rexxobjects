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
// File Name   : winshell.cpp
// Description : Win32 (Windows XP but perhaps not windows 98(?) etc)
//             : version of shellspawn
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

#include <windows.h>

#include <process.h> // for _beginthreadex

#include <io.h>    // for _get_osfhandle(), _isatty(), _fileno()

#include "shellspawn.h"

/* Suff to make it easy to call _beginthreadex */
typedef unsigned (__stdcall *PTHREAD_START) (void *);
#define safeCreateThread(psa, cbStack, pfnStartAddr, \
     pvParam, fdwCreate, pdwThreadID)                \
       ((HANDLE) _beginthreadex(                     \
          (void *) (psa),                            \
          (unsigned) (cbStack),                      \
          (PTHREAD_START) (pfnStartAddr),            \
          (void *) (pvParam),                        \
          (unsigned) (fdwCreate),                    \
          (unsigned *) (pdwThreadID)))

// Private structure to allow all the threads to share data etc. and
// make the shellspawn() call re-enterent
typedef struct _win32shelldata {
  vector<string>* vInput;  // data for input stream
  vector<string>* vOutput; // data for output stream
  vector<string>* vError;  // data for error stream
  string* sInput;          // data for input stream
  string* sOutput;         // data for output stream
  string* sError;          // data for error stream
  INHANDLER fInput;        // callback for input stream
  OUTHANDLER fOutput;      // callback for output stream
  OUTHANDLER fError;       // callback for error stream
  HANDLE hInputFile;
  HANDLE hOutputFile;
  HANDLE hErrorFile;
  int inThreadRC;          // Return code for the different threads
  string inThreadErrorText;
  int outThreadRC;
  string outThreadErrorText;
  int errThreadRC;
  string errThreadErrorText;
  PROCESS_INFORMATION ChildProcessInfo;
  /* Windows Handles for the pipes/streams */
  HANDLE hOutputReadTmp, hOutputRead, hOutputWrite;
  HANDLE hErrorReadTmp,  hErrorRead,  hErrorWrite;
  HANDLE hInputWriteTmp, hInputRead,  hInputWrite;
  /* Windows Handles for threads */
  HANDLE hInThread,  hOutThread,  hErrThread;
  DWORD  InThreadId, OutThreadId, ErrThreadId;
  /* Thread cmmunication */
  CRITICAL_SECTION* criticalsection;
  HANDLE callbackRequested;
  HANDLE callbackHandled;
  int callbackType; /* 1=StdIn, 2=StdOut or StdErr */
  OUTHANDLER callbackOutputHandler;      // function for output callbacks
  string* callbackBuffer;
  int callbackRC;
  void* context;
  bool needsConsole;
  bool createdConsole;
} SHELLDATA;

// Private functions
static DWORD WINAPI HandleInputThread(LPVOID lpvThreadParam);
static DWORD WINAPI HandleOutputThread(LPVOID lpvThreadParam);
static DWORD WINAPI HandleErrorThread(LPVOID lpvThreadParam);
static void Error(string context, string &errorText);
static void CleanUp(SHELLDATA* data);
static int WriteToStdin(string &line, SHELLDATA* data);
static void HandleOutputToVector(HANDLE hRead, vector<string>* vOut, int &error, string &errorText);
static void HandleOutputToString(HANDLE hRead, string* sOut, int &error, string &errorText);
static void HandleOutputToCallback(HANDLE hRead, OUTHANDLER fOut, int &error, string &errorText, SHELLDATA* data);
static void HandleStdinFromVector(SHELLDATA* data);
static void HandleStdinFromCallback(SHELLDATA* data);
static int HandleCallback(SHELLDATA* data, string &errorText);
static void NeedsNewConsole(FILE* file, SHELLDATA* data);
static void PressToContinue();

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
      // Needed so that only one callback is called at a time
      CRITICAL_SECTION criticalsection;

      // Create data structure - and make sure we make all the members empty
      SHELLDATA data;
      data.inThreadRC=0;
      data.inThreadErrorText.clear();
      data.outThreadRC=0;
      data.outThreadErrorText.clear();
      data.errThreadRC=0;
      data.errThreadErrorText.clear();
      data.ChildProcessInfo.hProcess = NULL;
      data.ChildProcessInfo.hThread = NULL;
      data.hInThread = NULL;
      data.hOutThread = NULL;
      data.hErrThread = NULL;
      data.hOutputRead = NULL;
      data.hOutputWrite = NULL;
      data.hErrorRead = NULL;
      data.hErrorWrite = NULL;
      data.hInputRead = NULL;
      data.hInputWrite = NULL;
      data.hInputFile = NULL;
      data.hOutputFile = NULL;
      data.hErrorFile = NULL;
      data.criticalsection = NULL;
      data.callbackRequested = NULL;
      data.callbackHandled = NULL;
      data.callbackType = 0;
      data.callbackOutputHandler = NULL;
      data.callbackBuffer = NULL;
      data.callbackRC = 0;
      data.context = context;
      data.needsConsole = false;
      data.createdConsole = false;

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

      // Do we need a console and do we need to create one? - these functions work it out
      NeedsNewConsole(pIn, &data);
      NeedsNewConsole(pOut, &data);
      NeedsNewConsole(pErr, &data);
      if (data.createdConsole) // We need to create a console
      {
        if (!AllocConsole())
        {
          // Error - try and clean-up
          Error("Failure W1 in AllocConsole() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
// TODO - Disable the close button(?)
      }

      // Clear any output strings
      if (data.vOutput) data.vOutput->clear();
      if (data.vError) data.vError->clear();
      if (data.sOutput) data.sOutput->clear();
      if (data.sError) data.sError->clear();

      // Do we need a critical section? i.e. Have we more than one callback ...
      if ( (fIn?1:0)+(fOut?1:0)+(fErr?1:0) > 1 ) {
        // Yes - so set it up
        // No Error reporting defined for the Win32 CriticalSection API - {sigh}
        data.criticalsection = &criticalsection;
        InitializeCriticalSection(data.criticalsection);
      }

      // Do we need the event handlers i.e. Have we any callbacks ...
      if ( (fIn?1:0)+(fOut?1:0)+(fErr?1:0) > 0 ) {
        data.callbackRequested = CreateEvent(
          NULL,         // default security attributes
          FALSE,        // auto-reset event
          FALSE,        // initial state is reset
          NULL          // No name needed
        );
        if (data.callbackRequested == NULL)
        {
          Error("Failure W2 in CreateEvent(callbackRequested) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        data.callbackHandled = CreateEvent(
          NULL,         // default security attributes
          FALSE,        // auto-reset event
          FALSE,        // initial state is reset
          NULL          // No name needed
        );
        if (data.callbackHandled == NULL)
        {
          Error("Failure W3 in CreateEvent(callbackHandled) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Set up the security attributes struct.
      SECURITY_ATTRIBUTES sa;
      sa.nLength= sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = NULL;
      sa.bInheritHandle = TRUE;

      // Create the output pipe and handles
      HANDLE temp;
      if (pOut)
      {
        // We have been given a FILE* stream so we want to make a handle from that
        // GetStdHandle is used for the case when we have created a console
        if (pOut==stdout) temp = GetStdHandle(STD_OUTPUT_HANDLE);
        else temp = (HANDLE)_get_osfhandle(_fileno(pOut));

        // We create a new duplicate handle to pass to the child so that
        // the child cannot close our handle
        if (!DuplicateHandle(GetCurrentProcess(), temp,
                             GetCurrentProcess(),
                             &data.hOutputFile, // Address of new handle.
                             0, TRUE,           // Make it inheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          // Error - try and clean-up
          Error("Failure W4 in DuplicateHandle(hOutputFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        temp = NULL;
      }
      else
      {
        // We Create a pipe
        if (!CreatePipe(&data.hOutputReadTmp,&data.hOutputWrite,&sa,0))
        {
          Error("Failure W5 in CreatePipe(output) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        // We create a new duplicate handle and set the Properties to FALSE
        // so the child does not inherit properties and, as a result,
        // a non-closeable handle to the pipe is created.
        // This is a better way - but it is not pre-XP compatable ....
        //   -> SetHandleInformation( hOutputRead, HANDLE_FLAG_INHERIT, 0);
        if (!DuplicateHandle(GetCurrentProcess(),data.hOutputReadTmp,
                             GetCurrentProcess(),
                             &data.hOutputRead, // Address of new handle.
                             0,FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          // Error - try and clean-up
          Error("Failure W6 in DuplicateHandle(hOutputRead) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        /* We dont want this closeable handle */
        if (!CloseHandle(data.hOutputReadTmp))
        {
          // Error - try and clean-up
          Error("Failure W7 in CloseHandle(output) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hOutputReadTmp = NULL;
      }

      // Create the standard error output pipe and handles
      if (pErr)
      {
        // We have been given a FILE* stream so we want to make a handle from that
        if (pErr==stderr) temp = GetStdHandle(STD_ERROR_HANDLE);
        else temp = (HANDLE)_get_osfhandle(_fileno(pErr));
        // Make a duplicate handle
        if (!DuplicateHandle(GetCurrentProcess(), temp,
                             GetCurrentProcess(),
                             &data.hErrorFile, // Address of new handle.
                             0, TRUE,          // Make it inheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          Error("Failure W8 in DuplicateHandle(hErrorFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        temp = NULL;
      }
      else
      {
        // We Create a pipe
        if (!CreatePipe(&data.hErrorReadTmp,&data.hErrorWrite,&sa,0))
        {
          // Error - try and clean-up
          Error("Failure W9 in CreatePipe(error) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        // Make a non-closeable handle to the pipe
        if (!DuplicateHandle(GetCurrentProcess(),data.hErrorReadTmp,
                             GetCurrentProcess(),
                             &data.hErrorRead, // Address of new handle.
                             0,FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          // Error - try and clean-up
          Error("Failure W10 in DuplicateHandle(hErrorRead) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        /* We dont want this closeable handle */
        if (!CloseHandle(data.hErrorReadTmp))
        {
          // Error - try and clean-up
          Error("Failure W11 in CloseHandle(error) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hErrorReadTmp = NULL;
      }

      // Create the child input pipe.
      if (pIn)
      {
        // We have been given a FILE* stream so we want to make a handle from that
        if (pIn==stdin) temp = GetStdHandle(STD_INPUT_HANDLE);
        else temp = (HANDLE)_get_osfhandle(_fileno(pIn));
        // Make a duplicate handle
        if (!DuplicateHandle(GetCurrentProcess(), temp,
                             GetCurrentProcess(),
                             &data.hInputFile, // Address of new handle.
                             0, TRUE,          // Make it inheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          Error("Failure W12 in DuplicateHandle(hInputFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        temp = NULL;
      }
      else
      {
        // We Create a pipe
        if (!CreatePipe(&data.hInputRead,&data.hInputWriteTmp,&sa,0))
        {
          // Error - try and clean-up
          Error("Failure W13 in CreatePipe(input) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        // Make a non-closeable handle to the pipe
        if (!DuplicateHandle(GetCurrentProcess(),data.hInputWriteTmp,
                             GetCurrentProcess(),
                             &data.hInputWrite, // Address of new handle.
                             0,FALSE, // Make it uninheritable.
                             DUPLICATE_SAME_ACCESS))
        {
          // Error - try and clean-up
          Error("Failure W14 in DuplicateHandle(hInputWrite) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }

        /* We dont want this closeable handle */
        if (!CloseHandle(data.hInputWriteTmp))
        {
          // Error - try and clean-up
          Error("Failure W15 in CloseHandle(input) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hInputWriteTmp = NULL;
      }

      // Launch the redirected command
      STARTUPINFO si;

      // Set up the start up info struct.
      ZeroMemory(&si,sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESTDHANDLES;

      if (data.hOutputFile) si.hStdOutput=data.hOutputFile;
      else si.hStdOutput = data.hOutputWrite;

      if (data.hInputFile) si.hStdInput = data.hInputFile;
      else si.hStdInput  = data.hInputRead;

      if (data.hErrorFile) si.hStdError = data.hErrorFile;
      else si.hStdError  = data.hErrorWrite;

      int flags;
      if (data.needsConsole) flags = 0; // I.e. inherit our console
      else flags = DETACHED_PROCESS;

      if (!CreateProcess(NULL,(CHAR*)command.c_str(),NULL,NULL,TRUE,
                         flags,NULL,NULL,&si,&data.ChildProcessInfo))
      {
        // Error - try and clean-up
        if (GetLastError() == 2) // File not found
        {
          CleanUp(&data);
          errorText = "The command was not found";
          return SHELLSPAWN_NOFOUND;
        }
        else
        {
          Error("Failure W16 in CreateProcess() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Launch the thread (if needed) that reads the childs standard output
      if (!data.hOutputFile)
      {
        data.hOutThread = safeCreateThread(NULL,0,HandleOutputThread,
                                (LPVOID)&data,0,&data.OutThreadId);

        if (data.hOutThread == NULL)
        {
          // Error - try and clean-up
          Error("Failure W17 in CreateThread(output) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Launch the thread (if needed) that reads the childs error output
      if (!data.hErrorFile)
      {
        data.hErrThread = safeCreateThread(NULL,0,HandleErrorThread,
                                (LPVOID)&data,0,&data.ErrThreadId);
        if (data.hErrThread == NULL)
        {
          // Error - try and clean-up
          Error("Failure W18 in CreateThread(error) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Launch the thread (if needed) that gets the input and sends it to the child.
      if (!data.hInputFile)
      {
        data.hInThread = safeCreateThread(NULL,0,HandleInputThread,
                                (LPVOID)&data,0,&data.InThreadId);
        if (data.hInThread == NULL)
        {
          // Error - try and clean-up
          Error("Failure W19 in CreateThread() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Handle callback request events (so that all callbacks are done in the
      // main thread - to make things easier for the calling process) while
      // waiting for the child process to exit
      if (data.callbackRequested)
      {
        HANDLE hEvents[2];
        hEvents[0]=data.ChildProcessInfo.hProcess;
        hEvents[1]=data.callbackRequested;
        bool processing = true;

        while (processing)
        {
          // Wait for the events
          int r = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
          switch (r)
          {
            case WAIT_OBJECT_0: // Process Finished
             processing = false;
             break;

            case WAIT_OBJECT_0 + 1: // Callback Request Signalled
             if (HandleCallback(&data, errorText)) return SHELLSPAWN_FAILURE;
             break;

            default:
              // Error - try and clean-up
              Error("Failure W20 in WaitForMultipleObjects(Process) in shellspawn()", errorText);
              CleanUp(&data);
              return SHELLSPAWN_FAILURE;
          }
        }
      }

      else // no callback handlers - so we just wait for the process to exit
      {
        // Wait for the child process to exit
        if (WaitForSingleObject(data.ChildProcessInfo.hProcess,INFINITE) == WAIT_FAILED)
        {
          // Error - try and clean-up
          Error("Failure W21 in WaitForSingleObject(process) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Get the child processes exit code
      DWORD ExitCode;
      if (!GetExitCodeProcess(data.ChildProcessInfo.hProcess, &ExitCode))
      {
        // Error - try and clean-up
        Error("Failure W22 in GetExitCodeProcess() in shellspawn()", errorText);
        CleanUp(&data);
        return SHELLSPAWN_FAILURE;
      }

      // Close process and thread handle
      if (!CloseHandle(data.ChildProcessInfo.hProcess))
      {
        // Error - try and clean-up
        Error("Failure W23 in CloseHandle(process) in shellspawn()", errorText);
        CleanUp(&data);
        return SHELLSPAWN_FAILURE;
      }
      data.ChildProcessInfo.hProcess = NULL;

      if (!CloseHandle(data.ChildProcessInfo.hThread))
      {
        // Error - try and clean-up
        Error("Failure W24 in CloseHandle(thread) in shellspawn()", errorText);
        CleanUp(&data);
        return SHELLSPAWN_FAILURE;
      }
      data.ChildProcessInfo.hThread = NULL;

      // Close pipe handles that are used by the redirected command
      // (should cause any ReadFile()s in the threads to exit)
      if (data.hOutputWrite)
      {
        if (!CloseHandle(data.hOutputWrite))
        {
          // Error - try and clean-up
          Error("Failure W25 in CloseHandle(OutputWrite) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hOutputWrite = NULL;
      }
      if (data.hErrorWrite)
      {
        if (!CloseHandle(data.hErrorWrite))
        {
          // Error - try and clean-up
          Error("Failure W26 in CloseHandle(ErrorWrite) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hErrorWrite = NULL;
      }
      if (data.hInputRead)
      {
        if (!CloseHandle(data.hInputRead))
        {
          // Error - try and clean-up
          Error("Failure W27 in CloseHandle(InputRead) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hInputRead = NULL;
      }

      // Wait for the Output thread to die.
      // If we have callback handleres we still have to check these as there may be some
      // pending activity triggered just before the process exited
      if (data.callbackRequested && data.hOutThread)
      {
        HANDLE hEvents[2];
        hEvents[0]=data.hOutThread;
        hEvents[1]=data.callbackRequested;
        bool processing = true;

        while (processing)
        {
          // Wait for the events
          int r = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
          switch (r)
          {
            case WAIT_OBJECT_0: // Process Finished
             processing = false;
             break;

            case WAIT_OBJECT_0 + 1: // Callback Request Signalled
             if (HandleCallback(&data, errorText)) return SHELLSPAWN_FAILURE;
             break;

            default:
              // Error - try and clean-up
              Error("Failure W28 in WaitForMultipleObjects(outputThread) in shellspawn()", errorText);
              CleanUp(&data);
              return SHELLSPAWN_FAILURE;
          }
        }
      }

      // Otherwsise we simply wait for the thread (if needed)
      else if (data.hOutThread)
      {
        if (WaitForSingleObject(data.hOutThread,INFINITE) == WAIT_FAILED)
        {
          // Error - try and clean-up
          Error("Failure W29 in WaitForSingleObject(OutThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Close thread handle
      if (data.hOutThread)
      {
        if (!CloseHandle(data.hOutThread))
        {
          // Error - try and clean-up
          Error("Failure W30 in CloseHandle(OutThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hOutThread = NULL;
      }

      // Wait for the error output thread to die.
      // If we have callback handleres we still have to check these as there may be some
      // pending activity triggered just before the process exited
      if (data.callbackRequested && data.hErrThread)
      {
        HANDLE hEvents[2];
        hEvents[0]=data.hErrThread;
        hEvents[1]=data.callbackRequested;
        bool processing = true;

        while (processing)
        {
          // Wait for the events
          int r = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
          switch (r)
          {
            case WAIT_OBJECT_0: // Process Finished
             processing = false;
             break;

            case WAIT_OBJECT_0 + 1: // Callback Request Signalled
             if (HandleCallback(&data, errorText)) return SHELLSPAWN_FAILURE;
             break;

            default:
              // Error - try and clean-up
              Error("Failure W31 in WaitForMultipleObjects(errorThread) in shellspawn()", errorText);
              CleanUp(&data);
              return SHELLSPAWN_FAILURE;
          }
        }
      }

      // Otherwsise we simply wait for the thread (if needed)
      else if (data.hErrThread)
      {
        if (WaitForSingleObject(data.hErrThread,INFINITE) == WAIT_FAILED)
        {
          // Error - try and clean-up
          Error("Failure W32 in WaitForSingleObject(ErrThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Close thread handle
      if (data.hErrThread)
      {
        if (!CloseHandle(data.hErrThread))
        {
          // Error - try and clean-up
          Error("Failure W33 in CloseHandle(ErrThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hErrThread = NULL;
      }

      // Wait for the input thread to die
      // If we have callback handleres we still have to check these as there may be some
      // pending activity triggered just before the process exited
      if (data.callbackRequested && data.hInThread)
      {
        HANDLE hEvents[2];
        hEvents[0]=data.hInThread;
        hEvents[1]=data.callbackRequested;
        bool processing = true;

        while (processing)
        {
          // Wait for the events
          int r = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
          switch (r)
          {
            case WAIT_OBJECT_0: // Process Finished
             processing = false;
             break;

            case WAIT_OBJECT_0 + 1: // Callback Request Signalled
             if (HandleCallback(&data, errorText)) return SHELLSPAWN_FAILURE;
             break;

            default:
              // Error - try and clean-up
              Error("Failure W34 in WaitForMultipleObjects(inputThread) in shellspawn()", errorText);
              CleanUp(&data);
              return SHELLSPAWN_FAILURE;
          }
        }
      }

      // Otherwsise we simply wait for the thread (if needed)
      else if (data.hInThread)
      {
        if (WaitForSingleObject(data.hInThread,INFINITE) == WAIT_FAILED)
        {
          // Error - try and clean-up
          Error("Failure W35 in WaitForSingleObject(InThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
      }

      // Close thread handle
      if (data.hInThread)
      {
        if (!CloseHandle(data.hInThread))
        {
          // Error - try and clean-up
          Error("Failure W36 in CloseHandle(InThread) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hInThread = NULL;
      }

      // Close pipe handles
      if (data.hOutputRead)
      {
        if (!CloseHandle(data.hOutputRead))
        {
          // Error - try and clean-up
          Error("Failure W37 in CloseHandle(OutputWrite) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hOutputRead = NULL;
      }
      if (data.hErrorRead)
      {
        if (!CloseHandle(data.hErrorRead))
        {
          // Error - try and clean-up
          Error("Failure W38 in CloseHandle(ErrorRead) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hErrorRead = NULL;
      }
      // NOTE: hInputWrite closed in HandleInputThread() below

      // Close the duplicate handles of the FILE* passed to use for the child process
      if (data.hOutputFile)
      {
        if (!CloseHandle(data.hOutputFile))
        {
          // Error - try and clean-up
          Error("Failure W39 in CloseHandle(hOutputFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hOutputFile = NULL;
      }
      if (data.hErrorFile)
      {
        if (!CloseHandle(data.hErrorFile))
        {
          // Error - try and clean-up
          Error("Failure W40 in CloseHandle(hErrorFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hErrorFile = NULL;
      }
      if (data.hInputFile)
      {
        if (!CloseHandle(data.hInputFile))
        {
          // Error - try and clean-up
          Error("Failure W41 in CloseHandle(hInputFile) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.hInputFile = NULL;
      }

      // Delete the critical section - if it was set-up
      // Note: no defined errors for Win32 Critical section API
      if (data.criticalsection) {
        DeleteCriticalSection(data.criticalsection);
        data.criticalsection = NULL;
      }

      if (data.callbackRequested)
      {
        if (!CloseHandle(data.callbackRequested))
        {
          // Error - try and clean-up
          Error("Failure W42 in CloseHandle(callbackRequested) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.callbackRequested = NULL;
      }

      if (data.callbackHandled)
      {
        if (!CloseHandle(data.callbackHandled))
        {
          // Error - try and clean-up
          Error("Failure W43 in CloseHandle(callbackHandled) in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.callbackHandled = NULL;
      }

      // Close the console if we created one
      if (data.createdConsole)
      {
        PressToContinue();
        if (!FreeConsole())
        {
          // Error - try and clean-up
          Error("Failure W44 in FreeConsole() in shellspawn()", errorText);
          CleanUp(&data);
          return SHELLSPAWN_FAILURE;
        }
        data.createdConsole = false;
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

      rc = (int)ExitCode;

 return SHELLSPAWN_OK;
}


void CleanUp(SHELLDATA* data)
{
  if (data->ChildProcessInfo.hProcess) {
    TerminateProcess(data->ChildProcessInfo.hProcess, 0);
    if (data->ChildProcessInfo.hProcess) CloseHandle(data->ChildProcessInfo.hProcess);
    if (data->ChildProcessInfo.hThread) CloseHandle(data->ChildProcessInfo.hThread);
  }
  if (data->hInThread) {
    TerminateThread(data->hInThread,0);
    CloseHandle(data->hInThread);
  }
  if (data->hOutThread) {
    TerminateThread(data->hOutThread,0);
    CloseHandle(data->hOutThread);
  }
  if (data->hErrThread) {
    TerminateThread(data->hErrThread,0);
    CloseHandle(data->hErrThread);
  }
  if (data->hOutputRead) CloseHandle(data->hOutputRead);
  if (data->hOutputWrite) CloseHandle(data->hOutputWrite);
  if (data->hErrorRead) CloseHandle(data->hErrorRead);
  if (data->hErrorWrite) CloseHandle(data->hErrorWrite);
  if (data->hInputRead) CloseHandle(data->hInputRead);
  if (data->hInputWrite) CloseHandle(data->hInputWrite);
  if (data->hInputFile) CloseHandle(data->hInputFile);
  if (data->hOutputFile) CloseHandle(data->hOutputFile);
  if (data->hErrorFile) CloseHandle(data->hErrorFile);
  data->hInputFile = NULL;
  data->hOutputFile = NULL;
  data->hErrorFile = NULL;
  if (data->criticalsection) {
    DeleteCriticalSection(data->criticalsection);
    data->criticalsection = NULL;
  }
  if (data->callbackRequested)
  {
    CloseHandle(data->callbackRequested);
    data->callbackRequested = NULL;
  }
  if (data->callbackHandled)
  {
    CloseHandle(data->callbackHandled);
    data->callbackHandled = NULL;
  }
  data->callbackType = 0;
  data->callbackOutputHandler = NULL;
  data->callbackBuffer = NULL;
  data->callbackRC = 0;
  if (data->createdConsole)
  {
   FreeConsole();
   data->createdConsole = false;
  }
}

// Routine to see if a FILE* descriptor implies that we need a create a new console
// Creates a console if need be
void NeedsNewConsole(FILE* file, SHELLDATA* data)
{
  HANDLE h, d;

  if (data->createdConsole) return; // Job already done

  if (file == NULL) return;    // Null - not relavent console not needed

  if (file == stdin) h = GetStdHandle(STD_INPUT_HANDLE);
  else if (file == stdout) h = GetStdHandle(STD_OUTPUT_HANDLE);
  else if (file == stderr) h = GetStdHandle(STD_ERROR_HANDLE);
  else return; // Not a std file at all ... console not needed

  if (h == NULL) data->createdConsole = true; // We need a console - but XP (at least) does not return this
  else if (h == INVALID_HANDLE_VALUE) data->createdConsole = true; // We need a console - but again XP (at least) does not return this
  else
  {
    /* OK Now we have to work out if the handle is actually valid ...
       DuplicateHandle fails on XP if not so we can test this           */
    if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(),
                               &d,                 // Address of new handle.
                               0, TRUE,            // Make it inheritable.
                               DUPLICATE_SAME_ACCESS))
    {
      data->createdConsole = true; // Error - so the handle is invalid - we need a new console
    }
    else
    {
      // The handle must be OK after all ...
      if (_isatty(_fileno(file))) data->needsConsole = true; // Seems to be a console device
      // else it has been redirected [to a file] ... so a console is not needed
      CloseHandle(d); // close the duplicate handle in any case
    }
  }
  if (data->createdConsole) data->needsConsole = true;
  return;
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
     errorText="Failure W45 in HandleCallback() in HandleCallback(). Internal Error: Unexpected callbackType";
     CleanUp(data);
     return -1;
  }

  // Cleanup
  data->callbackType = 0;
  data->callbackOutputHandler = NULL;
  data->callbackBuffer = NULL;

  // Signal the in, out or err thread that the callback has been handled
  if (!SetEvent(data->callbackHandled))
  {
    Error("Failure W46 in SetEvent(callbackHandled) in HandleCallback()", errorText);
    return -1;
  }

  return 0; // Success
}


/* Thread process to handle standard output */
DWORD WINAPI HandleOutputThread(LPVOID lpvThreadParam)
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
  return 0;
}

/* Thread process to handle standard error */
DWORD WINAPI HandleErrorThread(LPVOID lpvThreadParam)
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
  return 0;
}

/* Function to handle output to a vector of strings */
void HandleOutputToVector(HANDLE hRead, vector<string>* vOut, int &error, string &errorText)
{
      CHAR lpBuffer[256+1]; // Add one for a trailing null if needed
      DWORD nBytesRead;
      string *buffer=0;
      int start;
      bool reading = true;

      while(reading)
      {
         if (!ReadFile(hRead,lpBuffer,256,&nBytesRead,NULL) || !nBytesRead)
         {
            if (GetLastError() == ERROR_BROKEN_PIPE)
               reading = false;
            else {
              error = 1;
              Error("Failure W47 in Readfile() in HandleOutputToVector()", errorText);
              return;
            }
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
void HandleOutputToString(HANDLE hRead, string* sOut, int &error, string &errorText)
{
      CHAR lpBuffer[256+1]; // Add one for a trailing null if needed
      DWORD nBytesRead;
      bool reading = true;

      while(reading)
      {
         if (!ReadFile(hRead,lpBuffer,256,&nBytesRead,NULL) || !nBytesRead)
         {
            if (GetLastError() == ERROR_BROKEN_PIPE)
               reading = false;
            else {
              error = 1;
              Error("Failure W48 in Readfile() in HandleOutputToString()", errorText);
              return;
            }
         }
         if (sOut) { // if sOut is null discard output
            lpBuffer[nBytesRead]=0;
            sOut->append(lpBuffer);
         }
      }

      return;
}

/* Function to handle output to a callback */
void HandleOutputToCallback(HANDLE hRead, OUTHANDLER fOut, int &error,
                            string &errorText, SHELLDATA* data)
{
      CHAR lpBuffer[256+1]; // Add one for a trailing null if needed
      DWORD nBytesRead;
      bool reading = true;
      string buffer;

      while(reading)
      {
         if (!ReadFile(hRead,lpBuffer,256,&nBytesRead,NULL) || !nBytesRead)
         {
            if (GetLastError() == ERROR_BROKEN_PIPE)
               reading = false;
            else {
              error = 1;
              Error("Failure W49 in Readfile() in HandleOutputToCallback()", errorText);
              return;
            }
         }

         if (nBytesRead)
         {
           lpBuffer[nBytesRead]=0;
           buffer.append(lpBuffer);

           // Critical section is used to ensure that one callback is called at a time
           if (data->criticalsection) EnterCriticalSection(data->criticalsection);

           // OK we need to signal the main thread to do the callback for us so that all
           // callbacks run on the main thread - this helps the calling system
           // Set up the comon data
           data->callbackType = 2; // Output
           data->callbackOutputHandler = fOut;
           data->callbackBuffer = &buffer;
           // Signal the main thread
           if (!SetEvent(data->callbackRequested))
           {
             error = 1;
             Error("Failure W50 in SetEvent() in HandleOutputToCallback()", errorText);
             return;
           }
           // Wait for the main thread to have done the work
           if (WaitForSingleObject(data->callbackHandled, INFINITE) == WAIT_FAILED)
           {
             error = 1;
             Error("Failure W51 in WaitForSingleObject() in HandleOutputToCallback()", errorText);
             return;
           }

           if (data->criticalsection) LeaveCriticalSection(data->criticalsection);

           buffer.clear();
         }
      }

      return;
}


/* Thread process to handle standard input */
DWORD WINAPI HandleInputThread(LPVOID lpvThreadParam)
{
  SHELLDATA* data = (SHELLDATA*)lpvThreadParam;
  if (data->vInput)
         HandleStdinFromVector(data);

  else if (data->sInput)
         WriteToStdin(*(data->sInput), data);

  else if (data->fInput)
         HandleStdinFromCallback(data);

  // else  - Nothing to do ... just close the handle ... i.e. as below

  if (!CloseHandle(data->hInputWrite))
  {
    if (!data->inThreadRC) // don't overwrite an existing error
    {
      data->inThreadRC = 1;
      Error("Failure W52 in CloseHandle() in HandleInputThread()", data->inThreadErrorText);
    }
  }
  data->hInputWrite = NULL;
  return 0;
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
  bool childProcessWaitingForInput=true;
  while (childProcessWaitingForInput) {
    switch (WaitForSingleObject(data->hInputRead,0)) {  // No wait - return immediately

      case WAIT_FAILED:
        if (GetLastError() != ERROR_INVALID_HANDLE) {
          // Note: ERROR_INVALID_HANDLE just means the child process did not want stdin at all
          data->inThreadRC = 1;
          Error("Failure W53 in WaitForSingleObject() in HandleStdinFromCallback()", data->inThreadErrorText);
          return;
        }
        // Input Handle invalid - child process must have died
        childProcessWaitingForInput=false;
        break;

      case WAIT_OBJECT_0:
        // Pipe "Signalled" - Child might be finished
        // Check if the child is still alive - if it is it might try and read its
        // stdin later ... so we will loop round after waiting 10ms
        if (WaitForSingleObject(data->ChildProcessInfo.hProcess,10) != WAIT_TIMEOUT)
          // OK we can assume that the child process finished - even if this called errored
          childProcessWaitingForInput=false;
        break;

      case WAIT_TIMEOUT:
        // Timeout - so possibly the child is waiting for more input or it may not have
        // finished reading what is in the pipe's buffer.
        // To try and check this all we can do is wait a bit first to see if it the
        // pipe gets signalled.
        // This is fine if the callback is for human action - but rather slow for
        // automation ...
        // Also the 200ms wait is just a guess - the child process could still be reading
        // from the pipe buffer after 200ms ... but I failed to find any other way to do this.
        // Note: We can ignore an error here - and catch it next time round
        if (WaitForSingleObject(data->hInputRead,200)==WAIT_TIMEOUT) {
          // OK - still seems to be waiting
          // Get some data (probably a line) from the callback
          string line;

          // Critical section is used to ensure that one callback is called at a time
          // I.e. only one callback from in, out or err at a  time so that this
          // looks single threaded for the caller of shellspawn()
          // Note: no errors seem to be reported from the Win32 CriticalSection API
          if (data->criticalsection) EnterCriticalSection(data->criticalsection);

          // OK we need to signal the main thread to do the callback for us so that all
          // callbacks run on the main thread - this helps the calling system
          // Set up the comon data
          data->callbackType = 1; // Input
          data->callbackBuffer = &line;
          // Signal the main thread
          if (!SetEvent(data->callbackRequested))
          {
            data->inThreadRC = 1;
            Error("Failure W54 in SetEvent() in HandleStdinFromCallback()", data->inThreadErrorText);
            return;
          }
          // Wait for the main thread to have done the work
          if (WaitForSingleObject(data->callbackHandled, INFINITE) == WAIT_FAILED)
          {
            data->inThreadRC = 1;
            Error("Failure W55 in WaitForSingleObject() in HandleStdinFromCallback()", data->inThreadErrorText);
            return;
          }
          // Get the return code
          int callbackrc=data->callbackRC;
          // Cleanup
          data->callbackRC=0;

          if (data->criticalsection) LeaveCriticalSection(data->criticalsection);

          if ( callbackrc )
            // We were asked to kill the input stream - we just need to return
            return;

          // Write the line - exit on any error
          if (WriteToStdin(line, data)) return;
        }
        break;

      default:
        data->inThreadRC = 1;
        data->inThreadErrorText = "Failure W56 Unexpected result from Wait() in HandleInputThread()";
        return;
    }
  }
  return;
}


int WriteToStdin(string &line, SHELLDATA* data)
{
  int nTotalWrote=0;
  char* buff = (char*)line.c_str();
  int nBytes = line.length();
  DWORD nBytesWrote=0;
  while (nTotalWrote<nBytes)
  {
    if (!WriteFile(data->hInputWrite,(buff+nTotalWrote),(nBytes-nTotalWrote),&nBytesWrote, NULL))
    {
      if (GetLastError() == ERROR_NO_DATA) {
         // Pipe was closed, a normal exit path - the child exitted before processing all input
         // TODO: If need be we could see how much was read and update the line arg to reflect this
         //       but no caller of this function needed this feature (as yet)
         //       I.e. We could alter the string input method to make use of this (i.e. so the
         //       caller knows how much was read by the child process).
         return 1;
      }
      else {
        data->inThreadRC = 1;
        Error("Failure W57 in WriteFile() in WriteToStdin()", data->inThreadErrorText);
        return -1;
      }
    }
    nTotalWrote += nBytesWrote;
  }
  return 0;
}

void Error(string context, string &errorText)
{
       LPVOID lpvMessageBuffer;
       int rc = GetLastError();
       char sRC[10];
       itoa (rc,sRC,10);

       FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, rc,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpvMessageBuffer, 0, NULL);

       errorText = context + ". Win32 details: RC=" + sRC + " Text=" + (char *)lpvMessageBuffer;
       LocalFree(lpvMessageBuffer);
}

void PressToContinue()
{
  HANDLE hStdout, hStdin;
  LPSTR lpszPrompt = "Command Completed - Press ENTER to continue";
  CHAR chBuffer[256];
  DWORD cRead, cWritten;

  // Get handles to STDIN and STDOUT.
  hStdin = GetStdHandle(STD_INPUT_HANDLE);
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hStdin == INVALID_HANDLE_VALUE ||
      hStdout == INVALID_HANDLE_VALUE)
  {
    return;
  }

  // Write to STDOUT and read from STDIN
  if (! WriteFile(
          hStdout,              // output handle
          lpszPrompt,          // prompt string
          lstrlen(lpszPrompt), // string length
          &cWritten,            // bytes written
            NULL) )               // not overlapped
  {
    return;
  }

  ReadFile(
          hStdin,    // input handle
          chBuffer,  // buffer to read into
          255,       // size of buffer
          &cRead,    // actual bytes read
          NULL);     // not overlapped
};
