.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A ``GroupingWorkspace`` is a simple workspace with one value per
detector pixel; this value corresponds to the group number that will
be used when focussing or summing another workspace.

This algorithm creates a blank ``GroupingWorkspace``. It uses the
``InputWorkspace``, ``InstrumentName``, OR ``InstrumentFilename``
parameter to determine which Instrument to create.

If the ``OldCalFilename`` parameter is given, the ``.cal`` ASCII file
will be loaded to fill the group data.

If the ``GroupNames parameter`` is given, the names of banks matching
the comma-separated strings in the parameter will be used to
sequentially number the groups in the output.

If both the ``FixedGroupCount`` and ``ComponentName`` parameter are
given then the detectors for the given component will be grouped into
the number of groups specified, detectors will be left ungrouped in
the event that the number of detectors does not divide equally into
the number of groups.

If both the ``CustomGroupingString`` and ``ComponentName`` property are
given, then the detectors for the given component will be grouped using
this grouping string. The syntax of the ``CustomGroupingString`` is the
same as used in the :ref:`GroupDetectors <algm-GroupDetectors>` algorithm,
and is explained below:

.. code-block:: sh

  An example CustomGroupingString is 1,2+3,4-6,7:10

  The comma ',' separator denotes the different groups.
  The plus '+' separator means the adjoining detector IDs will be put in the same group.
  The dash '-' separator means the detector IDs inclusive to this range will be put in the same group.
  The colon ':' separator means the detector IDs inclusive to this range will be put in their own individual groups.

  The CustomGroupingString 1,2+3,4-6,7:10 therefore has the following detector grouping:

  Group 1: 1
  Group 2: 2 3
  Group 3: 4 5 6
  Group 4: 7
  Group 5: 8
  Group 6: 9
  Group 7: 10

``GroupDetectorsBy`` has five options

* ``All`` create one group for the whole instrument
* ``bank`` Creates one group per instrument component that starts with the word ``bank``
* ``Group`` Creates one group per instrument component that starts with the word ``Group``. POWGEN will create groups for ``North`` and ``South``. SNAP will create groups for ``East`` and ``West``.
* ``Column`` Creates one group per instrument component that starts with the word ``Column``
* ``2_4Grouping`` (SNAP only) creates one group of 4 columns of SNAP detectors and another with the remaining 2 columns. This grouping is used frequently in their reduction.

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
   print("Instrument name = {}".format(comp1.getName()))

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
   print("Instrument name = {}".format(comp1.getName()))

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
   print("Instrument name = {}".format(comp1.getName()))

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromIDF

   Instrument name = GEM

**Example - CreateGroupingWorkspace for IRIS graphite component**

.. testcode:: ExCreateGroupingWorkspaceFromComponent

   grouping_ws, spectra_count, group_count = CreateGroupingWorkspace(InstrumentName='IRIS', ComponentName='graphite', FixedGroupCount=5)

   print("Number of grouped spectra: {}".format(spectra_count))
   print("Number of groups: {}".format(group_count))

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromComponent

   Number of grouped spectra: 50
   Number of groups: 5

**Example - CreateGroupingWorkspace using a CustomGroupingString**

.. testcode:: ExCreateGroupingWorkspaceFromCustomGroupingString

   grouping_ws, spectra_count, group_count = CreateGroupingWorkspace(InstrumentName='IRIS', ComponentName='graphite', CustomGroupingString='3-25,26,27+28,29-35,36:53')

   print("Number of grouped spectra: {}".format(spectra_count))
   print("Number of groups: {}".format(group_count))

Output:

.. testoutput:: ExCreateGroupingWorkspaceFromCustomGroupingString

   Number of grouped spectra: 51
   Number of groups: 22

.. categories::

.. sourcelink::
