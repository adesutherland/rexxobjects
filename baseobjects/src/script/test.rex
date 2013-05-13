#!/usr/bin/rexx
/* Rexx Test File */
say "RexxObjects: BaseObjects and RXOScript Tests"

/******************************************************************************/
Say "Test 1 - Arguments to script"
parse arg a1, a2
say "Arg 1 is" a1
say "Arg 2 is" a2
/******************************************************************************/

/******************************************************************************/
Say "Test 2 - Address environemnt"
ret=address()
if ret!="RXOShell" then say "TEST 2.1 FAILED - Address() returned" ret
/******************************************************************************/

/******************************************************************************/
say "Test 3 - AddGlobalFilter"

ret=addglobalfilter() /* Should not work - no arguments */
if ret="OK" then say "TEST 3.1 FAILED - returned OK for a invalid arguments "

ret=addglobalfilter("FILTER", 3) /* Should not work - invlid arguments */
if ret="OK" then say "TEST 3.2 FAILED - returned OK for a invalid arguments "

ret=addglobalfilter("FILTER1") /* Should work */
if ret!="OK" then say "TEST 3.3 FAILED -" ret

ret=addglobalfilter("") /* Should work */
if ret!="OK" then say "TEST 3.4 FAILED -" ret

ret=addglobalfilter("1BAD") /* Should not work - invalid filter */
if ret="OK" then say "TEST 3.5 FAILED - returned OK for a invalid filter"
/******************************************************************************/

/******************************************************************************/
say "Test 4 - AddGlobalStem"

ret=addglobalstem() /* Should not work - no arguments */
if ret="OK" then say "TEST 4.1 FAILED - returned OK for a invalid arguments "

ret=addglobalstem("GSTEM.", 3) /* Should not work - invlid arguments */
if ret="OK" then say "TEST 4.2 FAILED - returned OK for a invalid arguments "

ret=addglobalstem("GSTEM.") /* Should work */
if ret!="OK" then say "TEST 4.3 FAILED -" ret

ret=addglobalstem("BAD") /* Should not work - invalid filter */
if ret="OK" then say "TEST 4.4 FAILED - returned OK for a invalid filter"
/******************************************************************************/

/******************************************************************************/
say "Test 5 - AddGlobalVar"

ret=addglobalvar() /* Should not work - no arguments */
if ret="OK" then say "TEST 5.1 FAILED - returned OK for a invalid arguments "

ret=addglobalvar("GVAR", 3) /* Should not work - invlid arguments */
if ret="OK" then say "TEST 5.2 FAILED - returned OK for a invalid arguments "

ret=addglobalvar("GVAR") /* Should work */
if ret!="OK" then say "TEST 5.3 FAILED -" ret

ret=addglobalvar("BAD.") /* Should not work - invalid filter */
if ret="OK" then say "TEST 5.4 FAILED - returned OK for a invalid filter"
/******************************************************************************/

/******************************************************************************/
say "Test 6 - SetRexx"

ret=setRexx() /* Should not work - no arguments */
if ret="OK" then say "TEST 6.1 FAILED - returned OK for a invalid arguments "

ret=setRexx("TestFunction", "arg a,b; return a+b;", 3) /* Should not work - invlid arguments */
if ret="OK" then say "TEST 6.2 FAILED - returned OK for a invalid arguments "

ret=setRexx("TestFunction", "arg a,b; return a+b;") /* Should work */
if ret!="OK" then say "TEST 6.3 FAILED -" ret

ret=TestFunction(1,2)
if ret!=3 then say "TEST 6.4 FAILED - Did not return 3! Returned" ret

/******************************************************************************/

/******************************************************************************/
say "Test 7 - CallFunction"

ret=CallFunction() /* Should not work - no arguments */
if ret="OK" then say "TEST 6.1 FAILED - returned OK for a invalid arguments "

ret=CallFunction("TestFunction", "arg a,b; return a+b;", 3) /* Should not work - invlid arguments */
if ret="OK" then say "TEST 6.2 FAILED - returned OK for a invalid arguments "

ret=CallFunction("TestFunction", "arg a,b; return a+b;") /* Should work */
if ret!="OK" then say "TEST 6.3 FAILED -" ret

/******************************************************************************/


Exit


say "Calling Direct"
"ls"

say "Explicit address rxoshell"
address rxoshell "ls"

say "Explicit address system"
address system "ls"

say "using pipe"
"ls > stem out."
do i=1 to out.0
 say "line" i "=" out.i
end

say "using pipe (in with address system)"
address system "ls > stem out."
do i=1 to out.0
 say "line" i "=" out.i
end

return 0
