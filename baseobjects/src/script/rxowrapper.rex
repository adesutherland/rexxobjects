/**************************************************************************
* A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
* *************************************************************************
* Work Name   : BaseObjects
* Description : Part or RexxObjects - Base C++ Objects for REXX.
* Copyright   : Copyright (C) 2009 Adrian Sutherland
* *************************************************************************
* A B O U T   T H I S   F I L E
* *************************************************************************
* File Name   : script/rxowrapper.rex
* Description : Rexx Script to wrap a RXO REXX Bootstrap script in a EXE
*             : Designed to be self-wrapped
* *************************************************************************
* L I C E N S E
* *************************************************************************
* This program is free software: you can redistribute it and/or modify
* it under the terms of version 3 of the GNU General Public License as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http:*www.gnu.org/licenses/>.
*
* For the avoidance of doubt:
* - Version 3 of the license (i.e. not earlier nor later versions) apply.
* - a copy of the license text should be in the "license" directory of the
*   source distribution.
* - Requests for use under other licenses will be treated sympathetically,
*   please see contact details.
* *************************************************************************
* C O N T A C T   D E T A I L S
* *************************************************************************
* E-mail      : adrian@sutherlandonline.org
*             : adrian@open-bpm.org
* Web         : http://rexxobjects.sourceforge.net/
*             : www.open-bpm.org
* Telephone   : Please e-mail for details
* Postal      : UK - Please e-mail for details
**************************************************************************/
say "RexxObjects Bootstrap Wrapper"

say "Version: 0.3"

/* Process arguments */
infile=""
outfile=""
config="rxowrapper.cfg"
if arg()>0 then infile=arg(1)
if arg()>1 then do
 if arg(2)="-c" | arg(2)="-C" then config=arg(3)
 else do
  outfile=arg(2)
  if arg()>2 then do
   if arg(3)="-c" | arg(3)="-C" then config=arg(4)
   else infile="" /* Force an error */
  end
 end
end

if infile="" then do
  say "Invalid Arguments"
  say "Format: rxowrapper infile [outfile] [-c config-file]"
  exit -1
end

parse version ver
say "Rexx Version:" ver

parse upper source os calltype filename

opsys="Unknown"
select
 when os="WINDOWSNT" then opsys="Windows"
 when os="WIN32" then opsys="Windows"
 otherwise do
  say "Unknown Operating System (" || os || "). Please fix me!"
  say "Please report this issue/fix to adrian@open-bpm.org"
 end
end

say "Platform:" opsys

/* Platform Configuration */
PLATFORM. = "UNDEFINED-COMMAND"
select
 when opsys="Windows" then do
  PLATFORM._RM = "cmd /c erase"
  PLATFORM._CPP = "g++.exe"
  PLATFORM._LD = "g++.exe"
  PLATFORM._STRIP = "strip.exe"
  PLATFORM._UPX = "upx.exe  --best" /* --ultra-brute" */
  PLATFORM._LINKOPS = "-mthreads" /* "-mwindows" */
  PLATFORM._CFLAGS = "-O3"
  PLATFORM._CLIBS = "-lwsock32 -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lmsimg32"
  PLATFORM._SHELLSPAWN = "-lshellspawn"
  PLATFORM._REXX = "-lrexxtrans"
  PLATFORM._BASEOBJECTS = "-lbaseobjects"
 end
 when opsys="Linux" then do
  PLATFORM._RM = "rm"
  PLATFORM._CPP = "g++"
  PLATFORM._LD = "g++"
  PLATFORM._STRIP = "strip"
  PLATFORM._UPX = "upx --best"
  PLATFORM._LINKOPS = ""
  PLATFORM._CFLAGS = "-O3"
  PLATFORM._CLIBS = ""
  PLATFORM._SHELLSPAWN = "-lshellspawn"
  PLATFORM._REXX = "-lrexxtrans"
  PLATFORM._BASEOBJECTS = "-lbaseobjects"
 end
 otherwise do
  say "WARNING - Unknown platform:" opsys
  say "No default commands - use rxowrapper.cfg"
 end
end

/* Exe Directory */
path=translate(getExeName(),"/","\")
pos=lastpos("/",path)
if pos=0 then path=""
else path = substr(path,1,pos-1) || "/"

/* Find rxowrapper.cfg */
found=0
if (lines(config)>0) then found=1
else do
  oldconfig=config
  config=path || config
  if (lines(config)>0) then found=1
end

/* Read rxowrapper.cfg */
if found then do
  say "Reading config file:" config
  signal on syntax
  do lineno=1 while lines(config)
   line=strip(linein(config))
   if line<>"" then interpret line
  end
  signal off syntax
end
else say "WARNING: Config files (" || oldconfig "or" config || ") not found"

parse var infile fn '.' ft

if ft="" then ft="rex"
infile = fn || "." || ft
cppfile=fn || ".cpp"
ofile=fn || ".o"

if outfile="" then do
 if opsys="Windows" then outfile=fn || ".exe"
 else outfile=fn
end

if  opsys="Windows" then do
  if translate(right(outfile,4)) <> ".EXE" then outfile=outfile || ".exe"
  outfile=translate(outfile,"\","/")
  infile=translate(infile,"\","/")
  cppfile=translate(cppfile,"\","/")
  ofile=translate(ofile,"\","/")
end

say "Input file:" infile
say "Output file:" outfile
say "CPP file:" cppfile
say "Object file:" ofile

/* Delete output file */
call execute PLATFORM._RM cppfile

say "Generating" cppfile
if lines(infile)=0 then do
  say "Input file not found:" infile
  exit -1
end

/* Create CPP file */
call lineout cppfile, "// This file was auto-generated by the ObjectRexx Wrapper preprocessor - DO NOT CHANGE"
call lineout cppfile, ""
call lineout cppfile, "#include <vector>"
call lineout cppfile, "#include <string>"
call lineout cppfile, "using namespace std;"
call lineout cppfile, "int bootstrapMain(string exeName, const char* bootstrap, vector<string> &args);"
call lineout cppfile, ""
call lineout cppfile, "int main(int argc, char **argv) {"

out= "  char* bootstrap=""" || makeCString(linein(infile)) || "\n"""
do while lines(infile)
 call lineout cppfile, out
 out="                  """ || makeCString(linein(infile)) || "\n"""
end
out=out || ";"
call lineout cppfile, out

call lineout cppfile, "  vector<string> args;"
call lineout cppfile, "  for (int i=1; i<argc; i++) args.push_back(argv[i]);"

call lineout cppfile, "  string exeName = argv[0];"

/* Make sure we have the .EXE - rough and ready - just checks if a 3 char file type is there */
if opsys="Windows" then do
 call lineout cppfile, "  if (exeName.length()<=4) exeName=exeName + "".exe"";"
 call lineout cppfile, "  else if (exeName.substr(exeName.length()-4,1).compare(""."")) exeName=exeName + "".exe"";"
end

call lineout cppfile, "  return bootstrapMain(exeName, bootstrap, args);"
call lineout cppfile, "};"
call lineout cppfile

/* Compile CPP file */
say "Compiling" cppfile
ret=execute(PLATFORM._CPP "-c" cppfile "-o" ofile PLATFORM._CFLAGS)
if ret then return

/* Build Exe */
say "Linking" outfile
ret = execute(PLATFORM._LD ofile "-o" outfile PLATFORM._LINKOPS PLATFORM._CLIBS PLATFORM._BASEOBJECTS PLATFORM._SHELLSPAWN PLATFORM._REXX)
if ret then return

/* Strip Exe */
if (PLATFORM._STRIP <> "") then do
  say "Stripping" outfile
  ret = execute(PLATFORM._STRIP outfile)
  if ret then return
end

/* Compress Exe */
if (PLATFORM._UPX <> "") then do
  say "Compressing" outfile
  call execute PLATFORM._UPX outfile
end

say "Cleanup"
call execute PLATFORM._RM cppfile ofile

say "Wrapper Completed"
return

/* Escape out any speach marks or back slashes */
makeCString: Procedure
 trace off
 parse arg in
 out="";
 do i=1 to length(in)
  c=substr(in,i,1)
  select
   when c="""" then out=out || "\"""
   when c="\" then out=out || "\\"
   otherwise out=out || c
  end
 end
return out

execute: procedure expose opsys
  parse arg cmd
  cmd ">STEM out. 2>STEM err. <RAWSTD"
  if RC\=0 then do
    say "ERROR when executing:" cmd
    say "RC="RC
    if out.0>0 then do
     say "Std Out"
     do i=1 to out.0
      say ">" out.i
     end
    end
    if err.0>0 then do
     say "Std Err"
     do i=1 to err.0
      say ">" err.i
     end
    end
    return 1
  end
return 0

syntax:
 say "Error in rxowrapper.cfg"
 say "Line number:" lineno
 say "Source:" line
 say "Error:" rc || "-" || errortext(rc)
Exit -1
