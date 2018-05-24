.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to save a GroupingWorkspace to XML file in a
format which is accepted by :ref:`algm-LoadDetectorsGroupingFile`.

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

.. categories::

.. sourcelink::
