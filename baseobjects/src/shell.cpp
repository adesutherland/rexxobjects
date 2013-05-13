// *************************************************************************
// A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
// *************************************************************************
// Work Name   : BaseObjects
// Description : Part or RexxObjects - Base C++ Objects for REXX.
// Copyright   : Copyright (C) 2008 Adrian Sutherland
// *************************************************************************
// A B O U T   T H I S   F I L E
// *************************************************************************
// File Name   : shell.cpp
// Description : Rexx SHELL Subcommand handler
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

#include <stdio.h>                     /* needed for printf()        */
#include <string.h>                    /* needed for strlen()        */
#include <stdlib.h>
#include <ctype.h>

#include <iostream>

#include <shellspawn.h>                // The shellspawn utility

#include "shell.h"                     // The RexxObjects shell header
#include "object.h"
#include "rexxfunc.h"                  // Rexx API interface


using namespace std;

static void HandleStdout(string data, void* context);
static void HandleStderr(string data, void* context);
static int  HandleStdin(string &data, void* );
static void RemoveCharRet(string &data);
static void RemoveCharRet(vector<string> &data);
static int SplitLine(string &data, string &firstLine);
static int ParseCommand(char* command, int &inMode,  string &inSource,
                                       int &outMode, string &outDest,
                                       int &errMode, string &errDest);
static int ParsePipes(char* pipes, int &inMode,  string &inSource,
                                   int &outMode, string &outDest,
                                   int &errMode, string &errDest);
static void ReadWord(char* input, int &p, string &word);
static bool isValidStem(string word);
static bool isValidVar(string word);

/* Class to hold the console line buffers */
class Buffers {
  public:
    string outbuffer; // line buffer for stdout
    string errbuffer; // line buffer for stderr
};


/* Error Codes */
// -1 = Command not found
// -2 = Command string format error (pipes)
// -9 = Internal Error
ULONG APIENTRY RXO_SHELL(
      PRXSTRING Command,  /* Command string passed from the caller */
      PUSHORT Flags,      /* pointer for return of flags */
      PRXSTRING Retstr)   /* pointer to RXSTRING for RC return */
{

  int inMode=0, outMode=0, errMode=0;
  string inSource, outDest, errDest;

  OUTHANDLER pfOut = NULL;
  OUTHANDLER pfErr = NULL;
  INHANDLER  pfIn = NULL;
  vector<string>* pvIn = NULL;
  vector<string>* pvOut = NULL;
  vector<string>* pvErr = NULL;
  string* psIn = NULL;
  string* psOut = NULL;
  string* psErr = NULL;
  FILE* fIn = NULL;
  FILE* fOut = NULL;
  FILE* fErr = NULL;

  vector<string> vIn, vOut, vErr;
  string sIn, sOut, sErr;

  Buffers consolebuffer;

  int rc;
  string spawnErrorText;

  PSZ command = Command->strptr; /* point to the command */

  /* Parse command string */
  if (ParseCommand(command, inMode, inSource, outMode, outDest, errMode, errDest))
  { // Failure
    rc = -2;
    *Flags = RXSUBCOM_FAILURE; /* this is a command failure */
    /* format return code string */
    sprintf(Retstr->strptr, "%d", rc);
    Retstr->strlength = strlen(Retstr->strptr);
    return 0;
  }

  // Setup the in stream
  switch (inMode)
  {
    case 0: // StdStream
      pfIn =  HandleStdin;
      break;

    case 1: // Var
      BRORexxFunction::GetRexxVariable(inSource, sIn);
      psIn = &sIn;
      break;

    case 2: // Stem
      BRORexxFunction::GetRexxVariable(inSource, vIn);
      pvIn = &vIn;
      break;

    case 4: // Raw Std
      fIn =  stdin;
      break;

    case 3: // Null
    default:
     break;
  }

  // Setup the out stream
  switch (outMode)
  {
    case 0: // StdStream
      pfOut =  HandleStdout;
      break;

    case 1: // Var
      psOut = &sOut;
      break;

    case 2: // Stem
      pvOut = &vOut;
      break;

    case 4: // Raw Std
      fOut =  stdout;
      break;

    case 3: // Null
    default:
     break;
  }

  // Setup the error stream
  switch (errMode)
  {
    case 0: // StdStream
      pfErr =  HandleStderr;
      break;

    case 1: // Var
      psErr = &sErr;
      break;

    case 2: // Stem
      pvErr = &vErr;
      break;

    case 4: // Raw Std
      fErr =  stderr;
      break;

    case 3: // Null
    default:
     break;
  }

  /* Spawn Command */
  int spawnErrorCode = shellspawn(command, pvIn, psIn, pfIn, fIn,
                              pvOut, psOut, pfOut, fOut,
                              pvErr, psErr, pfErr, fErr, rc,
                              spawnErrorText, (void*)&consolebuffer);
  if (spawnErrorCode) {
   BROBase::ErrorOut(spawnErrorText);
   if (spawnErrorCode == SHELLSPAWN_NOFOUND) rc = -1;
   else rc = -9;
   *Flags = RXSUBCOM_FAILURE; /* this is a command failure */
  }
  else if (rc!=0) *Flags = RXSUBCOM_ERROR; /* this is a command error */
  else *Flags = RXSUBCOM_OK;

  // Handle the output stream
  switch (outMode)
  {
    case 0: // StdStream
      // Write out any remaining output
      if (consolebuffer.outbuffer.size()>0)
      {
        BROBase::LineOut(consolebuffer.outbuffer);
        consolebuffer.outbuffer.clear();
      }
      break;

    case 1: // Var
      BRORexxFunction::SetRexxVariable(outDest, *psOut);
      break;

    case 2: // Stem
      RemoveCharRet(*pvOut);
      BRORexxFunction::SetRexxVariable(outDest, *pvOut);
      break;

    case 3: // Null
    case 4: // Raw STD
    default:
     break;
  }

  // Handle the error stream
  switch (errMode)
  {
    case 0: // StdStream
      if (consolebuffer.errbuffer.size()>0)
      {
        BROBase::ErrorOut(consolebuffer.errbuffer);
        consolebuffer.errbuffer.clear();
      }
      break;

    case 1: // Var
      BRORexxFunction::SetRexxVariable(errDest, *psErr);
      break;

    case 2: // Stem
      RemoveCharRet(*pvErr);
      BRORexxFunction::SetRexxVariable(errDest, *pvErr);
      break;

    case 3: // Null
    case 4: // Raw STD
    default:
     break;
  }

  /* format return code string */
  sprintf(Retstr->strptr, "%d", rc);
  Retstr->strlength = strlen(Retstr->strptr);

  return 0; /* processing completed */
}

void HandleStdout(string data, void* context)
{
  Buffers* consolebuffer = (Buffers*)context;
  RemoveCharRet(data);
  while (data.size()>0)
  {
    if (SplitLine(data, consolebuffer->outbuffer)) {
      // End of line found
      BROBase::LineOut(consolebuffer->outbuffer);
      consolebuffer->outbuffer.clear();
    }
    // else end of line not found the data has been added to buffer
  }
}

void HandleStderr(string data, void* context)
{
  Buffers* consolebuffer = (Buffers*)context;
  RemoveCharRet(data);
  while (data.size()>0)
  {
    if (SplitLine(data, consolebuffer->errbuffer)) {
      // End of line found
      BROBase::ErrorOut(consolebuffer->errbuffer);
      consolebuffer->errbuffer.clear();
    }
    // else end of line not found the data has been added to buffer
  }
}

int HandleStdin(string &data, void*)
{
  BROBase::GetInput(data);
  data.append("\n");
  return 0;
}

void RemoveCharRet(string &data)
{
  while (true)
  {
    size_t n = data.find( '\r', 0 );
    if (n == string::npos) break;
    data.erase(n, 1);
  }
}

static void RemoveCharRet(vector<string> &data)
{
  int size = data.size();
  for (int i=0; i<size; i++)
    RemoveCharRet(data[i]);
}

/* Appends the first line in data onto the end of firstLine
   Returns 1 if a newline was found 0 otherwise              */
int SplitLine(string &data, string &firstLine)
{
  size_t n = data.find( '\n', 0 );

  // No newline
  if (n == string::npos)
  {
    firstLine.append(data);
    data.clear();
    return 0;
  }
  firstLine.append(data,0,n);
  data.erase(0, n+1);
  return 1;
}

/* Parse the command to get the pipe redirects */
int ParseCommand(char* command, int &inMode,  string &inSource,
                            int &outMode, string &outDest,
                            int &errMode, string &errDest)
{
  int rc=0;
  int l=0;

  // Skip Leading Spaces
  for (; command[l]; l++) if (command[l]!=' ') break;

  // Program bin/exe name
  for (; command[l]; l++) if (command[l]==' ') break;

  // Arguments
  for (; command[l]; l++)
  {
    switch (command[l])
    {
      case '"':
       // Read to the end of the string
       for (l++; command[l]; l++) if (command[l]=='"') break;
       break;

      case '2':
       if (command[l+1]!='>') break;
      case '>':
      case '<':
       // OK process the pipe args
       // Parse the rest of the string holding the pipes
       rc=ParsePipes(command + l, inMode, inSource, outMode, outDest, errMode, errDest);
       command[l]=0; // Truncate the command
       return rc;
      default: ;
    }
  }
  return 0;
}

/* Parse the pipe string to get redirects */
/* Modes 0=std stream, 1=var, 2=stem, 3=null */
int ParsePipes(char* pipes, int &inMode,  string &inSource,
                            int &outMode, string &outDest,
                            int &errMode, string &errDest)
{
  int l=0;
  string type;
  string dest;
  int stream=0; // 1=in, 2=out, 3=err
  // Skip Leading Spaces
  for (; pipes[l]; l++) if (pipes[l]!=' ') break;

  // Anything to do?
  if (!pipes[l]) return 0;

  // Get stream type
  switch (pipes[l])
  {
    case '<':
      stream = 1; // Stdin
      break;
    case '>':
      stream = 2; // Stdout
      break;
    case '2':
      if (pipes[l+1]=='>')
      {
        l++;
        stream = 3; // Stderr
      }
      break;
    default: ;
  }
  if (!stream)
  {
    string error = "Invalid Pipe: ";
    error.append(pipes + 1);
    BROBase::ErrorOut(error);
    return 1;
  }
  l++;

  // Get the source or destination
  ReadWord(pipes, l, type);
  if (type=="VAR")
  {
     ReadWord(pipes, l, dest);
     if (!isValidVar(dest))
     {
       BROBase::ErrorOut("Invalid Rexx Variable: " + dest);
       return 1;
     }
     switch (stream)
     {
       case 1:
         inMode=1; /* Var */
         inSource=dest;
         break;
       case 2:
         outMode=1;
         outDest=dest;
         break;
       case 3:
         errMode=1;
         errDest=dest;
     }
  }

  else if (type=="STEM")
  {
     ReadWord(pipes, l, dest);
     if (!isValidStem(dest))
     {
       BROBase::ErrorOut("Invalid Rexx Stem Name: " + dest);
       return 1;
     }
     switch (stream)
     {
       case 1:
         inMode=2; /* Stem */
         inSource=dest;
         break;
       case 2:
         outMode=2;
         outDest=dest;
         break;
       case 3:
         errMode=2;
         errDest=dest;
     }
  }

  else if (type=="NULL")
  {
     switch (stream)
     {
       case 1:
         inMode=3; /* Null */
         break;
       case 2:
         outMode=3;
         break;
       case 3:
         errMode=3;
     }
  }

  else if (type=="RAWSTD")
  {
     switch (stream)
     {
       case 1:
         inMode=4; /* Raw Std */
         break;
       case 2:
         outMode=4;
         break;
       case 3:
         errMode=4;
     }
  }


  else
  {
    BROBase::ErrorOut("Invalid Pipe Source or Destination: " + type);
    return 1;
  }

  // Handle more pipe redirects?
  return ParsePipes(pipes+l, inMode, inSource, outMode, outDest, errMode, errDest);
}

void ReadWord(char* input, int &p, string &word)
{
  string ch;
  word.clear();

  // Skip Spaces
  for (; input[p]; p++) if (input[p]!=' ') break;

  // Read word
  for (; input[p]; p++)
  {
    if (input[p]==' ') break;
    ch = toupper(input[p]);
    word.append(ch);
  }
}

static bool isValidStem(string word)
{
  int len = word.size();
  if (!len) return false;

  if (word[len-1]!='.') return false;

  if (!isalpha(word[0])) return false;

  for (int i=1; i<len-1; i++)
    if (!isalnum(word[i])) return false;

  return true;
}

static bool isValidVar(string word)
{
  int len = word.size();
  if (!len) return false;

  if (!isalpha(word[0])) return false;

  for (int i=1; i<len; i++)
    if (!isalnum(word[i])) return false;

  return true;
}
