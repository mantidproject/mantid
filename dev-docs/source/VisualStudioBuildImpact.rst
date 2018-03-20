Visual Studio Build Impact
==========================

Building Mantid on Visual Studio can take a while, and really tie up the
computer. This is because Visual Studio starts all of the compilation
processes at normal priority, so they compete as equals with everything
you are doing.

So you might think that you can open task manage and reduce the priority
of all the cl.exe processes, well there can be quite a few, and no it
won't work for long as these processes are replaced for every file it
compiles.

What you want to do it hunt down the MSBuild.exe processes and reduce
the priority of them, there should be the same number as you have
logical processors. The MSBuild processes spawn all of the compiler and
linker tasks, an they inherit the priority of the MSBuild process.

Script
------

Of course if you don't want to do this yourself then you can use this
script.

::

    Const BELOW_NORMAL = 16384
     
    strComputer = "."
    Set objWMIService = GetObject("winmgmts:\\" & strComputer & "\root\cimv2")

    Set colProcesses = objWMIService.ExecQuery _
        ("Select * from Win32_Process Where Name = 'MSBuild.exe'")
    For Each objProcess in colProcesses
        objProcess.SetPriority(BELOW_NORMAL) 
    Next

Save it as Reduce_Build_Impact.vbs, and use when things are running like
a dog!

Monitoring Script
-----------------

If you don't want to keep running the script for each build, here's one
that keeps a watch on your system every 5 seconds.

::

    Const BELOW_NORMAL = 16384
     
    strComputer = "."
    Set objWMIService = GetObject("winmgmts:\\" & strComputer & "\root\cimv2")
    Do While true
      Set colProcesses = objWMIService.ExecQuery _
          ("Select * from Win32_Process Where Name = 'MSBuild.exe'")
      For Each objProcess in colProcesses
          objProcess.SetPriority(BELOW_NORMAL) 
      Next
      WScript.Sleep 5000
    loop
