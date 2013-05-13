/* AVRexx (Another Visual Rexx) - (c) Adrian Sutherland 2006 */
/* Form Build Preprocessor                                   */
parse arg file, cppfile

call RxFuncAdd 'SysFileDelete', 'rexxutil', 'SysFileDelete'

/*
parse var file fn '.'
cppfile = fn || '.cpp'
*/

form. = ""
form.0 = 0;

/* Read and process file */
do ln=1 while lines(file)
 line=linein(file)
 parse var line cmd data
 cmd=strip(cmd)
 data=strip(data)
 select
  when left(cmd,1)='#' then iterate
  when cmd="" then iterate
  when cmd='BEGINFORM' then do
   n=form.0+1; form.0=n;
   call ProcessForm n, data
  end
  otherwise do
   say "Error 0: Invalid Line"
   say "         Line number:" || ln
   say "         Line:" line
   exit 1
  end
 end
end

call SysFileDelete cppfile
call WriteCPP

exit 0

ProcessForm: procedure expose file ln form.
 trace off
 parse arg base, name
 form.base.0=0

 form.base._cname = "avr_form_" || translate(name,'_',' ')
 form.base._fname = "avr_factory_" || translate(name,'_',' ')
 form.base._rname = "avr_callback_" || translate(name,'_',' ')
 form.base._name = translate(name,'_',' ')

 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="WIDTH" then form.base._width=data
   when cmd="HEIGHT" then form.base._height=data
   when cmd="TITLE" then form.base._title=data
   when cmd="GLOBALVAR" then form.base._globalvar = form.base._globalvar data
   when cmd="GLOBALSTEM" then form.base._globalstem = form.base._globalstem data
   when cmd="GLOBALFILTER" then form.base._globalfilter = form.base._globalfilter data
   when cmd='BEGINLABEL' then do
    n=form.base.0+1; form.base.0=n; child=base || '.' || n
    call ProcessLabel child, data
   end
   when cmd='BEGINBUTTON' then do
    n=form.base.0+1; form.base.0=n; child=base || '.' || n
    call ProcessButton child, data
   end
   when cmd='BEGINFUNCTION' then do
    n=form.base.0+1; form.base.0=n; child=base || '.' || n
    call ProcessFunction child, data
   end
   when cmd='BEGINCALLBACK' then do
    child=base || '._CALLBACK'
    call ProcessCallback child, data
   end
   when cmd='ENDFORM' then return
   otherwise do
    say "Error 1: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 8: Unexpected End of File"
 exit 1

return

ProcessLabel: procedure expose file ln form.
 trace off
 parse arg base, name
 form.base.0=0

 form.base._type = "LABEL"
 form.base._cname = "avr_label_" || translate(name,'_',' ')
 form.base._rname = "avr_callback_" || translate(name,'_',' ')
 form.base._name = translate(name,'_',' ')

 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="WIDTH" then form.base._width=data
   when cmd="HEIGHT" then form.base._height=data
   when cmd="LEFT" then form.base._left=data
   when cmd="TOP" then form.base._top=data
   when cmd="TITLE" then form.base._title=data
   when cmd="BOXTYPE" then form.base._boxtype=data
   when cmd="LABELFONT" then form.base._labelfont=data
   when cmd="LABELSIZE" then form.base._labelsize=data
   when cmd="LABELTYPE" then form.base._labeltype=data
   when cmd="GLOBALVAR" then form.base._globalvar = form.base._globalvar data
   when cmd="GLOBALSTEM" then form.base._globalstem = form.base._globalstem data
   when cmd="GLOBALFILTER" then form.base._globalfilter = form.base._globalfilter data
   when cmd='BEGINCALLBACK' then do
    child=base || '._CALLBACK'
    call ProcessCallback child, data
   end
   when cmd='BEGINFUNCTION' then do
    n=form.base.0+1; form.base.0=n; child=base || '.' || n
    call ProcessFunction child, data
   end

   when cmd="ENDLABEL" then return
   otherwise do
    say "Error 2: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 7: Unexpected End of File"
 exit 1

return

ProcessButton: procedure expose file ln form.
 trace off
 parse arg base, name
 form.base.0=0

 form.base._type = "BUTTON"
 form.base._cname = "avr_button_" || translate(name,'_',' ')
 form.base._rname = "avr_callback_" || translate(name,'_',' ')
 form.base._name = translate(name,'_',' ')

 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="WIDTH" then form.base._width=data
   when cmd="HEIGHT" then form.base._height=data
   when cmd="LEFT" then form.base._left=data
   when cmd="TOP" then form.base._top=data
   when cmd="TITLE" then form.base._title=data
   when cmd="GLOBALVAR" then form.base._globalvar = form.base._globalvar data
   when cmd="GLOBALSTEM" then form.base._globalstem = form.base._globalstem data
   when cmd="GLOBALFILTER" then form.base._globalfilter = form.base._globalfilter data
   when cmd='BEGINCALLBACK' then do
    child=base || '._CALLBACK'
    call ProcessCallback child, data
   end
   when cmd='BEGINFUNCTION' then do
    n=form.base.0+1; form.base.0=n; child=base || '.' || n
    call ProcessFunction child, data
   end
   when cmd="ENDBUTTON" then return
   otherwise do
    say "Error 3: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 6: Unexpected End of File"
 exit 1

return

ProcessFunction: procedure expose file ln form.
 trace off
 parse arg base, name
 form.base.0=0

 form.base._type = "FUNCTION"
 form.base._name = translate(translate(name,'_',' '))

 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="EXTERNALREXX" then form.base._externalrexx=data
   when cmd="REXX" then form.base._rexx = ProcessREXX();
   when cmd="ENDFUNCTION" then return
   otherwise do
    say "Error 7: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 8: Unexpected End of File"
 exit 1
return

ProcessCallback: procedure expose file ln form.
 trace off
 parse arg base, name
 form.base.0=0

 form.base._type = "CALLBACK"
 form.base._name = translate(name,'_',' ')

 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="EXTERNALREXX" then form.base._externalrexx=data
   when cmd="REXX" then form.base._rexx = ProcessREXX();
   when cmd="ENDCALLBACK" then do
    trace off
    return
   end
   otherwise do
    say "Error 9: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 10: Unexpected End of File"
 exit 1
return

ProcessREXX: procedure expose file ln
 trace off
 rexx=""
 do while lines(file)
  ln=ln+1
  line=linein(file)
  parse var line cmd data
  cmd=strip(cmd)
  data=strip(data)
  select
   when left(cmd,1)='#' then iterate
   when cmd="" then iterate
   when cmd="LINE" then rexx=rexx || data || X2C('0A')
   when cmd="ENDREXX" then return rexx
   otherwise do
    say "Error 4: Invalid Line"
    say "         Line number:" || ln
    say "         Line:" line
    exit 1
   end
  end
 end

 say "Error 5: Unexpected End of File"
 exit 1

return


WriteCPP: procedure expose cppfile form.
 trace off
 call lineout cppfile, "// This file was auto-generated by the AVRexx preprocessor - DO NOT CHANGE"
 call lineout cppfile, ""
 call lineout cppfile, "#include ""debug.h""" /* FOR DEBUG */
 call lineout cppfile, "#include ""form.h"""

 /* Process Required Headers */
 headers.=0;
 do f=1 to form.0
  do i=1 to form.f.0
   type=form.f.i._type
   if headers.type=1 then iterate
   headers.type=1
   select
    when type="BUTTON" then call lineout cppfile, "#include ""button.h"""
    when type="LABEL" then call lineout cppfile, "#include ""label.h"""
    otherwise nop
   end
  end
 end

 /* Process Forms */
 do f=1 to form.0
  call lineout cppfile, ""
  call WriteForm f
 end
 call lineout cppfile
return


WriteForm: procedure expose cppfile form.
 trace off
 parse arg base

 /********************************************************************/
 /* HEADER INFOMRATION                                               */
 /********************************************************************/

 /* Form Class */
 call lineout hfile, ""
 call lineout cppfile, "class" form.base._cname ": public AVRForm {"
 call lineout cppfile, " private:"
 do i=1 to form.base.0
  select
   when form.base.i._type="LABEL" then do
    call lineout cppfile, "  AVRLabel*" form.base.i._cname";"
   end
   when form.base.i._type="BUTTON" then do
    call lineout cppfile, "  AVRButton*" form.base.i._cname";"
   end
   when form.base.i._type="FUNCTION" then nop
   when form.base.i._type="CALLBACK" then nop
   otherwise do
    say "Internal Error 1: Invalid Type:" form.base.i._type
    exit 1
   end
  end
 end
 call lineout cppfile, "  void buildForm();"
 call lineout cppfile, " public:"
 call lineout cppfile, "  "form.base._cname"(AVRObject* parent, std::string name);"
 call lineout cppfile, "  "form.base._cname"(AVRObject* parent, std::string name, std::vector<std::string> &args);"
 call lineout cppfile, "};"

 /* Factory Class */
 call lineout cppfile, ""
 call lineout cppfile, "class" form.base._fname ": public AVRObjectFactory {"
 call lineout cppfile, " public:"
 call lineout cppfile, "  "form.base._fname"();"
 call lineout cppfile, "  AVRObject* newObject(AVRObject* parent, std::string name);"
 call lineout cppfile, "  AVRObject* newObject(AVRObject* parent, std::string name, std::vector<std::string> &args);"
 call lineout cppfile, "};"
 call lineout cppfile, ""

 /********************************************************************/
 /* IMPLEMENTATION                                                   */
 /********************************************************************/

 /* Form Constructor */
 call lineout cppfile, ""
 call lineout cppfile, form.base._cname"::"form.base._cname"(AVRObject* prnt, std::string nm) : AVRForm(prnt, nm, "form.base._width"," form.base._height"," form.base._title")"
 call lineout cppfile, "{"
 call lineout cppfile, "  LOG("""form.base._cname"::"form.base._cname"(parent, name)"");" /* DEBUG */
 call lineout cppfile, "  buildForm();"
 call lineout cppfile, "}"
 call lineout cppfile, form.base._cname"::"form.base._cname"(AVRObject* prnt, std::string nm, std::vector<std::string> &args) : AVRForm(prnt, nm, "form.base._width"," form.base._height"," form.base._title")"
 call lineout cppfile, "{"
 call lineout cppfile, "  LOG("""form.base._cname"::"form.base._cname"(parent, name, args)"");" /* DEBUG */
 call lineout cppfile, "  buildForm();"
 call lineout cppfile, "}"
 call lineout cppfile, ""

 /* Form Builder */
 call lineout cppfile, ""
 call lineout cppfile, "void" form.base._cname"::buildForm() {"
 call lineout cppfile, "  LOG("""form.base._cname"::buildForm()"");" /* DEBUG */
 call lineout cppfile, "  type="""form.base._name""";"

 call lineout cppfile, "  beginNewObject();"

 /* Handle Globals */
 do i=1 to words(form.base._globalvar)
  global = word(form.base._globalvar,i)
  call lineout cppfile, "  addGlobalVar(""" || global || """);"
 end
 do i=1 to words(form.base._globalstem)
  global = word(form.base._globalstem,i)
  call lineout cppfile, "  addGlobalStem(""" || global || """);"
 end
 do i=1 to words(form.base._globalfilter)
  global = word(form.base._globalfilter,i)
  call lineout cppfile, "  addGlobalFilter(""" || global || """);"
 end

 /* Handle child objects */
 do i=1 to form.base.0
  call writeChild base || '.' || i, ""
 end


 /* Handle Form Callback */
 if form.base._callback._rexx <> "" then do
   call lineout cppfile, "  setCallbackWithStatic("
   call split form.base._callback._rexx, 60
   do x=1 to line.0
     ln = "           """makeCString(line.x)
     if x=line.0 then ln=ln || """);"
     else ln=ln || """"
     call lineout cppfile, ln
   end
 end
 else if form.base._callback._externalrexx <> "" then do
  call lineout cppfile, "  setExternalCallback("""form.base._callback._externalrexx""");"
 end

 call lineout cppfile, "  endNewObject();"

 call lineout cppfile, "  show();"
 call lineout cppfile, "};"

 /* Factory Constructor */
 call lineout cppfile, ""
 call lineout cppfile, form.base._fname"::"form.base._fname"() : AVRObjectFactory("""form.base._name""") {};"

 /* Factory Functions */
 call lineout cppfile, ""
 call lineout cppfile, "AVRObject*" form.base._fname"::newObject(AVRObject* parent, std::string name)"
 call lineout cppfile, "{"
 call lineout cppfile, "  return new" form.base._cname"(parent, name);"
 call lineout cppfile, "};"

 call lineout cppfile, ""
 call lineout cppfile, "AVRObject*" form.base._fname"::newObject(AVRObject* parent, std::string name, std::vector<std::string> &args)"
 call lineout cppfile, "{"
 call lineout cppfile, "  return new" form.base._cname"(parent, name, args);"
 call lineout cppfile, "};"

 /* Factory Instance */
 call lineout cppfile, ""
 call lineout cppfile, "static" form.base._fname "factory;"
return

WriteChild: procedure expose cppfile form.
 trace off
 parse arg base, object
 if object<>"" then object=object || "->"

 select
  when form.base._type="LABEL" then do
   call lineout cppfile, "  "form.base._cname "= new AVRLabel(this,"""form.base._name""", "form.base._left"," form.base._top"," ,
        form.base._width"," form.base._height", "form.base._title");"
   if form.base._boxtype<>"" then
    call lineout cppfile, "  "form.base._cname"->boxType(fltk::"form.base._boxtype");"
   if form.base._labelfont<>"" then
    call lineout cppfile, "  "form.base._cname"->labelFont(fltk::"form.base._labelfont");"
   if form.base._labelsize<>"" then
    call lineout cppfile, "  "form.base._cname"->labelSize("form.base._labelsize");"
   if form.base._labeltype<>"" then
    call lineout cppfile, "  "form.base._cname"->labelType(fltk::"form.base._labeltype");"
  end

  when form.base._type="BUTTON" then do
   call lineout cppfile, "  "form.base._cname "= new AVRButton(this,"""form.base._name""","form.base._left"," form.base._top"," ,
        form.base._width"," form.base._height", "form.base._title");"
  end

  when form.base._type="FUNCTION" then do
   if form.base._rexx <> "" then do
    call lineout cppfile, "  " || object || "setRexxWithStatic("""form.base._name""","
    call split form.base._rexx, 60
    do x=1 to line.0
      ln = "           """makeCString(line.x)
      if x=line.0 then ln=ln || """);"
      else ln=ln || """"
      call lineout cppfile, ln
    end
   end
   else if form.base._externalrexx <> "" then do
     call lineout cppfile, "  " || object || "setExternalRexx("""form.base._name""", """form.base._externalrexx""");"
   end
  end

  otherwise do
   say "Internal Error 2: Invalid Type:" form.base._type
   exit 1
  end

 end

 /* Handle Callbacks */
 if form.base._callback._rexx <> "" then do
   call lineout cppfile, "  "form.base._cname"->setCallbackWithStatic("
   call split form.base._callback._rexx, 60
   do x=1 to line.0
     ln = "           """makeCString(line.x)
     if x=line.0 then ln=ln || """);"
     else ln=ln || """"
     call lineout cppfile, ln
   end
 end
 else if form.base._callback._externalrexx <> "" then do
  call lineout cppfile, "  "form.base._cname"->setExternalCallback("""form.base._callback._externalrexx""");"
 end

 /* Handle Globals */
 do i=1 to words(form.base._globalvar)
  global = word(form.base._globalvar,i)
  call lineout cppfile, "  "form.base._cname || "->addGlobalVar(""" || global || """);"
 end
 do i=1 to words(form.base._globalstem)
  global = word(form.base._globalstem,i)
  call lineout cppfile, "  "form.base._cname || "->addGlobalStem(""" || global || """);"
 end
 do i=1 to words(form.base._globalfilter)
  global = word(form.base._globalfilter,i)
  call lineout cppfile, "  "form.base._cname || "->addGlobalFilter(""" || global || """);"
 end

 /* Handle child Items */
 if form.base.0>0 then do
  childobject = form.base._cname
  /* call lineout cppfile, "  " || childobject || "->beginNewObject();" */
  do i=1 to form.base.0
   call writeChild base || '.' || i, childobject
  end
  /* call lineout cppfile, "  " || childobject || "->endNewObject();" */
 end

return

Split: procedure expose line.
 trace off
 parse arg line, length
 line.0=0
 do while length(line)>length
  x=line.0+1; line.0=x;
  line.x=left(line,length);
  line=substr(line,length+1);
 end
 if length(line)>0 then do
  x=line.0+1; line.0=x; line.x=line
 end
return

/* Escape out any speach marks or back slashes */
makeCString: Procedure
 trace off
 parse arg in
 out="";
 nl=X2C('0A')
 do i=1 to length(in)
  c=substr(in,i,1)
  select
   when c="""" then out=out || "\"""
   when c="\" then out=out || "\\"
   when c=nl then out=out || "\n"
   otherwise out=out || c
  end
 end
return out
