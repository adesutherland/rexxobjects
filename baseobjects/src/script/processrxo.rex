/**************************************************************************
* A B O U T   T H I S   W O R K  -  B A S E O B J E C T S
* *************************************************************************
* Work Name   : BaseObjects
* Description : Part or RexxObjects - Base C++ Objects for REXX.
* Copyright   : Copyright (C) 2009 Adrian Sutherland
* *************************************************************************
* A B O U T   T H I S   F I L E
* *************************************************************************
* File Name   : script/processrxo.rex
* Description : Process RXO file to run or generate RexxObjects Programs
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
say "RexxObjects RXO Pre-Processor"

say "Version: 0.3"

/* Process arguments */
file=""
outfile=""
config="processrxo.cfg"
if arg()>0 then file=arg(1)
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

if file="" then do
  say "Invalid Arguments"
  say "Format: processrxo infile [outfile] [-c config-file]"
  say ""
  say "If outfile is specified then then an exe is produced with RXOWRAPPER"
  say "otherwise the program is executed."
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
  PLATFORM._SCRIPT = "rxoscript"
  PLATFORM._WRAP = "rxowrapper"
  PLATFORM._WRAPCONFIG = ""
 end
 when opsys="Linux" then do
  PLATFORM._RM = "rm"
  PLATFORM._SCRIPT = "rxoscript"
  PLATFORM._WRAP = "rxowrapper"
  PLATFORM._WRAPCONFIG = ""
 end
 otherwise do
  say "WARNING - Unknown platform:" opsys
  say "No default commands - use processrxo.cfg"
 end
end

/* Find processrxo.cfg */
found=0
if (lines(config)>0) then found=1
else do
  path=translate(getExeName(),"/","\")
  pos=lastpos("/",path)
  if pos=0 then path=""
  else path = substr(path,1,pos-1) || "/"
  oldconfig=config
  config=path || config
  if (lines(config)>0) then found=1
end

/* Read processrxo.cfg */
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

parse var file fn '.' ft

if ft="" then ft="rxo"
file=fn || '.' || ft
rexxfile=fn || ".rex"

if  opsys="Windows" then do
  if outfile<>"" & translate(right(outfile,4)) <> ".EXE" then outfile=outfile || ".exe"
  outfile=translate(outfile,"\","/")
  file=translate(file,"\","/")
  rexxfile=translate(rexxfile,"\","/")
end


say "Input file:" file
say "REXX file:" rexxfile
say "EXE file:" outfile

/* Delete output file */
call execute PLATFORM._RM rexxfile

say "Generating Bootstrap REXX File"
if lines(file)=0 then do
  say "Input file not found:" file
  exit -1
end

object. = ""
object.0 = 0;

/* Read and process file */
do ln=1 while lines(file)>0
 line=linein(file)
 parse var line cmd data
 cmd=translate(strip(cmd))
 data=strip(data)
 select
  when left(cmd,1)='#' then iterate
  when cmd="" then iterate
  when cmd='BEGINOBJECT' then do
   n=object.0+1; object.0=n;
   call ProcessObject n, data
  end
  otherwise do
   /* TODO - extention point? */
   say "Error 1: Invalid Line"
   say "         Line number:" || ln
   say "         Line:" line
   exit 1
  end
 end
end

call WriteOut

if outfile="" then do
  say "Executing Bootstrap Rexx with RXOSCRIPT"
  say ""
  say "------------------------------------------------------------------------"
  PLATFORM._SCRIPT rexxfile
  say "------------------------------------------------------------------------"
  say ""
end
else do
  say "Wrapping Bootstrap Rexx to" outfile
  say ""
  say "------------------------------------------------------------------------"
  PLATFORM._WRAP rexxfile outfile PLATFORM._WRAPCONFIG "<RAWSTD"
  say "------------------------------------------------------------------------"
  say ""
end

say "Cleanup"
call execute PLATFORM._RM rexxfile

exit

ProcessObject: procedure expose file ln object.
 trace off
 parse arg base, name
 object.base.0=0
 object.base._name = translate(name,'_',' ')
 object.base._type = "BROBase"
 object.base._function.0=0

 do while lines(file)>0
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=translate(strip(cmd))
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="TYPE" then object.base._type=translate(data,'_',' ')
   when cmd='BEGINOBJECT' then do
    n=object.base.0+1; object.base.0=n; child=base || '.' || n
    call ProcessObject child, data
   end
   when cmd="GLOBALVAR" then object.base._globalvar = object.base._globalvar data
   when cmd="GLOBALSTEM" then object.base._globalstem = object.base._globalstem data
   when cmd="GLOBALFILTER" then object.base._globalfilter = object.base._globalfilter data
   when cmd='BEGINFUNCTION' then do
    n=object.base._function.0+1; object.base._function.0=n; child=base || '._FUNCTION.' || n
    call ProcessFunction child, data
   end
   when cmd='ENDOBJECT' then return
   otherwise do
    /* TODO - This is where extentions need to be handled with plugins - somehow */
    say "Error 2: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end

  end
 end

 say "Error 3: Unexpected End of File"
 exit 1

return


ProcessFunction: procedure expose file ln object.
 trace off
 parse arg base, name

 object.base._name = translate(translate(name,'_',' '))
 object.base._rexx.0=0

 do while lines(file)>0
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=translate(strip(cmd))
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="REXXFILE" then call ProcessREXXFile base || "._REXX", data
   when cmd="REXX" then call ProcessREXX base || "._REXX"
   when cmd="ENDFUNCTION" then do
     if object.base._rexx.0=0 then do
       say "Error 4: No REXX code for function"
       say "         Line number:" || ln
       say "         Line:" line
       exit 1
     end
     return
   end
   otherwise do
    say "Error 5: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 6: Unexpected End of File"
 exit 1
return


ProcessREXX: procedure expose file ln object.
 trace off
 parse arg base

 do while lines(file)>0
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=translate(strip(cmd))
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="LINE" then do
     r=object.base.0+1; object.base.0=r; object.base.r=data
   end
   when cmd="ENDREXX" then return
   otherwise do
    say "Error 6: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 8: Unexpected End of File"
 exit 1

return


ProcessREXXFile: procedure expose ln line object.
 trace off
 parse arg base, rexxFile
 if lines(rexxFile)=0 then do
   say "Error 9: REXX file not found:" rexxFile
   say "         Line number:" || ln
   say "         Line:" line
   exit 1
 end
 do while lines(rexxFile)>0
   r=object.base.0+1; object.base.0=r; object.base.r=linein(rexxFile)
 end
return


WriteOut: procedure expose rexxfile object. mainClass
 trace off
 mainClass=""
 call lineout rexxfile, "/* This file was auto-generated by the ProcessRXO preprocessor - DO NOT CHANGE */"
 call lineout rexxfile, ""

 /* Process Objects */
 do f=1 to object.0
  call lineout rexxfile, ""
  call WriteObject f, ""
 end

 if mainClass="" then do
   say "WARNING - A root object with a MAIN function has not been defined - code will not start"
 end

 call lineout rexxfile, 'return "' || mainClass || '"'
 call lineout rexxfile
return


WriteObject: procedure expose rexxfile object. mainClass
 trace off
 parse arg base, parent

 /* Create Object */
 if parent="" then do
   myname=object.base._name
   call lineout rexxfile, "call NewRootObject """ || object.base._type || """,""" || object.base._name || """"
 end
 else do
   myname=parent || "." || object.base._name
   call lineout rexxfile, "call ." || parent || ".NewObject """ || object.base._type || """,""" || object.base._name || """"
 end

 /* Handle Globals */
 do i=1 to words(object.base._globalvar)
  global = word(object.base._globalvar,i)
  call lineout rexxfile, "call ." || myname || ".addGlobalVar """ || global || """"
 end
 do i=1 to words(object.base._globalstem)
  global = word(object.base._globalstem,i)
  call lineout rexxfile, "call ." || myname || ".addGlobalStem """ || global || """"
 end
 do i=1 to words(object.base._globalfilter)
  global = word(object.base._globalfilter,i)
  call lineout rexxfile, "call ." || myname || ".addGlobalFilter """ || global || """"
 end

 /* Handle Functions */
 do f=1 to object.base._function.0
   n=object.base._function.f._name
   if parent="" & translate(n)="MAIN" then do
     if mainClass="" then mainClass=myname
     else do
       say "WARNING Duplicate root objects with a MAIN function have been defined"
       say "        Using object" mainClass "ignoring" myname
     end
   end
   do x=1 to object.base._function.f._rexx.0
     ln=""
     r=makeRexxString(object.base._function.f._rexx.x)
     if x=1 then ln="_func=" || r
     else ln="      " || r
     if x<object.base._function.f._rexx.0 then ln=ln || " || X2C('0A') ||,"
     call lineout rexxfile, ln
   end
   call lineout rexxfile, "call ." || myname || ".SetRexx """ || n || """,_func"
 end

 /* Handle child objects */
 do i=1 to object.base.0
  call writeObject base || '.' || i, myname
 end
return


/* Escape out any speach marks or back slashes */
makeRexxString: Procedure
 trace off
 parse arg in
 out="";
 do i=1 to length(in)
  c=substr(in,i,1)
  select
   when c='"' then out=out || '""'
   otherwise out=out || c
  end
 end
return '"' || out || '"'


execute: procedure
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
 say "Error in processrxo.cfg"
 say "Line number:" lineno
 say "Source:" line
 say "Error:" rc || "-" || errortext(rc)
Exit -1
