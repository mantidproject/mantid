.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to save a GroupingWorkspace to XML file or a MAP file.
The XML file format is the same as that accepted by :ref:`algm-LoadDetectorsGroupingFile`.
The MAP file format for detector grouping is as follows:

.. code-block:: sh

  Line 1         The number of groups.

  For N = 2 to N = Number of groups:

  Line N         The Group ID.
  Line N + 1     The number of Detector IDs in the group.
  Line N + 2     A space-separated list of Detector IDs in this group.

Usage
-----

**Example - Saving a custom grouping for MUSR instrument:**

.. testcode:: ExMUSRGrouping

   import os

   result = CreateGroupingWorkspace(InstrumentName = 'MUSR')

   grouping = result[0]

   # Add spectra 1 - 16 to group 1
   for i in range(0,16):
     grouping.dataY(i)[0] = 1

   # Add spectra 17 - 33 to group 2
   for i in range(16,33):
     grouping.dataY(i)[0] = 2

   # Spectra 34 - 64 are left in group 0, i.e. are left unused

   save_path = os.path.join(config["defaultsave.directory"], "musr_det_grouping.xml")

   SaveDetectorsGrouping(grouping, save_path)

   with open(save_path, 'r') as f:
     print(f.read().replace('\t', '  ').strip())

Output:

.. testoutput:: ExMUSRGrouping

   <?xml version="1.0"?>
   <detector-grouping instrument="MUSR">
     <group ID="0">
       <detids>34-64</detids>
     </group>
     <group ID="1">
       <detids>1-16</detids>
     </group>
     <group ID="2">
       <detids>17-33</detids>
     </group>
   </detector-grouping>

.. testcleanup:: ExMUSRGrouping

   os.remove(save_path)

**Example - Saving a custom grouping to a MAP file:**

.. testcode:: ExMAPFileGrouping

   import os

   # Create the custom detector grouping workspace
   grouping_ws, _, _ = CreateGroupingWorkspace(InstrumentName='IRIS', ComponentName='graphite', CustomGroupingString='3:5,6+7+8,9,10-20,21-35,36-53')

   # Specify the save location
   save_path = os.path.join(config["defaultsave.directory"], "iris_detector_grouping.map")

   # Save the detector grouping to a MAP file
   SaveDetectorsGrouping(InputWorkspace="grouping_ws", OutputFile=save_path)

   with open(save_path, 'r') as f:
     print(f.read().replace('\t', '  ').strip())

Output:

.. testoutput:: ExMAPFileGrouping

   8
   1
   1
   3
   2
   1
   4
   3
   1
   5
   4
   3
   6 7 8
   5
   1
   9
   6
   11
   10 11 12 13 14 15 16 17 18 19 20
   7
   15
   21 22 23 24 25 26 27 28 29 30 31 32 33 34 35
   8
   18
   36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53

.. testcleanup:: ExMAPFileGrouping

   os.remove(save_path)

.. categories::

.. sourcelink::
