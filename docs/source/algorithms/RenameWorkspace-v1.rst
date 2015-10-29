.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Renames a workspace to a different name in the data service. If the same
name is provided for input and output then the algorithm will fail with
an error. The Renaming is implemented as a removal of the original
workspace from the data service and re-addition under the new name.

If run on a group workspace, the members of the group will be renamed if
their names follow the pattern groupName\_1, groupName\_2, etc. (they
will be renamed to newName\_1, newname\_2, etc.). Otherwise, only the
group itself will be renamed - the members will keep their previous
names.



Usage
-----

**Example**

.. testcode:: ExRenameWorkspace

   myWs=CreateSampleWorkspace()
   mon_ws = CreateSampleWorkspace() 
   myWs.setMonitorWorkspace(mon_ws)  

   print "myWs name:", myWs.name()
   print "myWs exists in Mantid?", mtd.doesExist(myWs.name())

   newNameWs = RenameWorkspace(myWs)

   print "newNameWs name:", newNameWs.name()
   print "newNameWs exists in Mantid?", mtd.doesExist(newNameWs.name())

   print "As myWs is just a refence to the workspace it now refers to the workspace with the new name"
   print "myWs name:", myWs.name()
   print "Does 'myWs' exist in Mantid?", mtd.doesExist("myWs")
   #
   print "Does 'mon_ws' exist in Mantid",mtd.doesExist("mon_ws")   
   print "***************************************************"   
   print "Renaming workspace and monitors workspace together:"
   #
   newName1Ws = RenameWorkspace(myWs,RenameMonitors=True)
   #
   print "**** Renamed workspaces exist in mtd:"
   print "Does 'newName1Ws' exist in Mantid?", mtd.doesExist("newName1Ws")
   print "Does 'newName1Ws_monitors' exist in Mantid?", mtd.doesExist("newName1Ws_monitors") 
   print "**** old workspaces have disappeared"
   print "Does 'newNameWs' exist in Mantid?", mtd.doesExist("newNameWs")
   print "Does 'mon_ws' exist in Mantid?", mtd.doesExist("mon_ws") 
   # 
   mon_ws1 = newName1Ws.getMonitorWorkspace()
   print "The name of the monitor workspace attached to workspace:",\
            newName1Ws.name(),"\nIs: ",\
            mon_ws1.name()

Output:

.. testoutput:: ExRenameWorkspace

    myWs name: myWs
    myWs exists in Mantid? True
    newNameWs name: newNameWs
    newNameWs exists in Mantid? True
    As myWs is just a refence to the workspace it now refers to the workspace with the new name
    myWs name: newNameWs
    Does 'myWs' exist in Mantid? False
    Does 'mon_ws' exist in Mantid True
    ***************************************************
    Renaming workspace and monitors workspace together:
    **** Renamed workspaces exist in mtd:
    Does 'newName1Ws' exist in Mantid? True
    Does 'newName1Ws_monitors' exist in Mantid? True
    **** old workspaces have disappeared
    Does 'newNameWs' exist in Mantid? False
    Does 'mon_ws' exist in Mantid? False
    The name of the monitor workspace attached to workspace: newName1Ws 
    Is:  newName1Ws_monitors

.. categories::

.. sourcelink::
