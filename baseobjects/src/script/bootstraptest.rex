/* Compile Bootstrap */
parse arg a
say "Bootstrap Test"
say "Program Name:" getExeName()
say "Current Directory:" Directory()
say "Arg:" a

parse version ver
say "Rexx Version:" ver

call NewRootObject "BROBase", "testapp"

call .testapp.setrexx "main","say ""In App - Should only find TESTAPP root below:""; call listroot ""root.""; do i=1 to root.0; say root.i; end; say ""returning 5""; return 5"

return "TestApp"
