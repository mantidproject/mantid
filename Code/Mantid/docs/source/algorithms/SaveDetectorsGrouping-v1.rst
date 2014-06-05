.. algorithm::

.. summary::

.. alias::

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

   # Add spectra 0 - 15 to group 1
   for i in range(0,16):
     grouping.dataY(i)[0] = 1

   # Add spectra 16 - 32 to group 2
   for i in range(16,33):
     grouping.dataY(i)[0] = 2

   # Spectra 33 - 64 are left in group 0, i.e. are left unused

   save_path = os.path.join(config["defaultsave.directory"], "musr_det_grouping.xml")

   SaveDetectorsGrouping(grouping, save_path)

.. testcleanup:: ExMUSRGrouping

   os.remove(save_path)

.. categories::
