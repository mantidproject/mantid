.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A `GroupingWorkspace <http://www.mantidproject.org/GroupingWorkspace>`_ is a simple workspace with
one value per detector pixel; this value corresponds to the group number
that will be used when focussing or summing another workspace.

This algorithm creates a blank GroupingWorkspace. It uses the
InputWorkspace, InstrumentName, OR InstrumentFilename parameterto
determine which Instrument to create.

If the OldCalFilename parameter is given, the .cal ASCII file will be
loaded to fill the group data.

If the GroupNames parameter is given, the names of banks matching the
comma-separated strings in the parameter will be used to sequentially
number the groups in the output.

If both the FixedGroupCount and ComponentName parameter are given then
the detectors for the given component will be grouped into the number
of groups specified, detectors will be left ungrouped in the event that
the number of detectors does not divide equally into the number of groups.

Usage
-----

**Example - CreateGroupingWorkspace for MUSR Instrument**

.. include:: ../usagedata-note.txt 

.. testcode:: ExCreateGroupingWorkspaceSimple

   # Run algorithm with instrument specified
   result = CreateGroupingWorkspace(InstrumentName="MUSR")

   # Confirm instrument in grouping workspace.
   grouping = result[0]
   inst1 = grouping.getInstrument()
   comp1 = inst1.getComponentByName("MUSR")
   print "Instrument name =", comp1.getName()

Output:

.. testoutput:: ExCreateGroupingWorkspaceSimple

   Instrument name = MUSR

**Example - CreateGroupingWorkspace from MUSR workspace**

.. testcode:: ExCreateGroupingWorkspaceFromWorkspace

   # Create Workspace
   load_result = Load("MUSR00015189")
   group = load_result[0]
   ws_1 = group[0]

   # Run algorithm with workspace
   result = CreateGroupingWorkspace(InputWorkspace=ws_1)

   # Confirm instrument in grouping workspace.
   grouping = result[0]
   inst1 = grouping.getInstrument()
   comp1 = inst1.getComponentByName("MUSR")
   print "Instrument name =", comp1.getName() 

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromWorkspace

   Instrument name = MUSR

**Example - CreateGroupingWorkspace from GEM Instrument Definition**

.. testcode:: ExCreateGroupingWorkspaceFromIDF

   # Run algorithm with Instrument Definition File
   result = CreateGroupingWorkspace(InstrumentFilename="GEM_Definition.xml")

   # Confirm instrument in grouping workspace.
   grouping = result[0]
   inst1 = grouping.getInstrument()
   comp1 = inst1.getComponentByName("GEM")
   print "Instrument name =", comp1.getName()

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromIDF

   Instrument name = GEM
   
**Example - CreateGroupingWorkspace for IRIS graphite component**

.. testcode:: ExCreateGroupingWorkspaceFromComponent

   grouping_ws, spectra_count, group_count = CreateGroupingWorkspace(InstrumentName='IRIS', ComponentName='graphite', FixedGroupCount=5)
  
   print "Number of grouped spectra:",spectra_count
   print "Number of groups:",group_count

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromComponent

   Number of grouped spectra: 50
   Number of groups: 5

.. categories::
