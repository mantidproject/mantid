.. algorithm::

.. summary::

.. relatedalgorithms::

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

The new name can be the same as any existing workspaces if the overwrite flag
is set to true. This will replace the existing workspace.
If the Overwrite flag is set to false and any name is in use RenameWorkspace
will not rename the workspace and log an error.

.. warning::
   RenameWorkspace is set to overwrite by default to maintain backwards compatibility. 
   Ensure the overwrite flag is set to false to prevent any existing workspaces being 
   replaced with the renamed workspace.


Usage
-----

**Example**

.. testcode:: ExRenameWorkspace

   AnalysisDataService.clear()
   myWs=CreateSampleWorkspace()
   mon_ws = CreateSampleWorkspace() 
   myWs.setMonitorWorkspace(mon_ws)  
   print("*********************************************************************")
   print("{0:20}|{1:>20}|{2:>20}|".format("Existing WS names: ",myWs.name(),mon_ws.name()))
   obj_inADS = AnalysisDataService.getObjectNames()
   obj_inADS.sort()
   print("{0:20}|{1:>6}| With Names: |{2:>20}|{3:>20}|".format("Exist in ADS: ",len(obj_inADS),obj_inADS[0],obj_inADS[1]))
   
   NameA = RenameWorkspace(myWs)
   
   print("***** After simple rename:")
   print("{0:20}|{1:>20}|{2:>20}|".format("Existing WS names: ",NameA.name(),mon_ws.name()))
   obj_inADS = AnalysisDataService.getObjectNames()
   obj_inADS.sort()
   print("{0:20}|{1:>6}| With Names: |{2:>20}|{3:>20}|".format("Exist in ADS: ",len(obj_inADS),obj_inADS[0],obj_inADS[1]))

   print("Old pointer to myWs refers to workspace with new name:  {}".format(myWs.name()))
   print("*********************************************************************")
   print("***** After renaming workspace and monitors workspace together:")
   
   NameB = RenameWorkspace(myWs,RenameMonitors=True)
   
   print("{0:20}|{1:>20}|{2:>20}|".format("Existing WS names: ",NameB.name(),mon_ws.name()))
   obj_inADS = AnalysisDataService.getObjectNames()
   obj_inADS.sort()
   print("{0:20}|{1:>6}| With Names: |{2:>20}|{3:>20}|".format("Exist in ADS: ",len(obj_inADS),obj_inADS[0],obj_inADS[1]))
    
   mon_ws1 = NameB.getMonitorWorkspace()
   print("The name of the monitor workspace attached to workspace:{0:>6}| Is:  {1:>10}|".
            format(NameB.name(),mon_ws1.name()))
   print("*********************************************************************")
             
Output:

.. testoutput:: ExRenameWorkspace

    *********************************************************************
    Existing WS names:  |                myWs|              mon_ws|
    Exist in ADS:       |     2| With Names: |              mon_ws|                myWs|
    ***** After simple rename: 
    Existing WS names:  |               NameA|              mon_ws|
    Exist in ADS:       |     2| With Names: |               NameA|              mon_ws|
    Old pointer to myWs refers to workspace with new name:  NameA
    *********************************************************************
    ***** After renaming workspace and monitors workspace together:
    Existing WS names:  |               NameB|      NameB_monitors|
    Exist in ADS:       |     2| With Names: |               NameB|      NameB_monitors|
    The name of the monitor workspace attached to workspace: NameB| Is:  NameB_monitors|
    *********************************************************************
    
**Example - Setting Overwrite on and off:**

.. testcode:: ExOverwriteExisting

   #Clear the ADS before starting
   AnalysisDataService.clear()
       
   #Create an existing workspace called 'wsOld'
   CreateWorkspace([0], [0], OutputWorkspace="wsOld")
       
   #Next create a workspace we are going to rename
   CreateWorkspace([0], [0], OutputWorkspace="wsNew")
       
   #This will fail telling us that 'wsOld' already exists
   print('Trying to rename with OverwriteExisting set to false.')
   try:
       RenameWorkspace(InputWorkspace="wsNew", OutputWorkspace="wsOld", OverwriteExisting=False)
   except RuntimeError:
       print('RuntimeError: The workspace wsOld already exists')
       
   #This will succeed in renaming and 'wsOld' will be replaced with 'wsNew'
   print('Trying to rename with OverwriteExisting set to true.')
   RenameWorkspace(InputWorkspace="wsNew", OutputWorkspace="wsOld", OverwriteExisting=True) 
   print('Succeeded')
   
Output:

.. testoutput:: ExOverwriteExisting

   Trying to rename with OverwriteExisting set to false.
   RuntimeError: The workspace wsOld already exists
   Trying to rename with OverwriteExisting set to true.
   Succeeded

   
.. categories::

.. sourcelink::
