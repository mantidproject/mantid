.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. figure:: /images/InstrumentTree.jpg
   :alt: Instrument Tree

   Instrument Tree

Create a :ref:`calibration file <CalFile>` for diffraction focusing based
on list of names of the instrument tree.

If a new file name is specified then offsets in the file are all sets to
zero and all detectors are selected. If a valid calibration file already
exists at the location specified by the :ref:`GroupingFileName <CalFile>`
then any existing offsets and selection values will be maintained and
only the grouping values changed.

Detectors not assigned to any group will appear as group 0, i.e. not
included when using the DiffractionFocussing algorithm.

The group number is assigned based on a descent in the instrument tree
assembly. If two assemblies are parented, say Bank1 and module1, and
both assembly names are given in the GroupNames, they will get assigned
different grouping numbers. This allows to isolate a particular
sub-assembly of a particular leaf of the tree.

Usage
-----
**Example - create cal file from GEM instrument**

.. include:: ../usagedata-note.txt

.. testcode:: ExCreateCalFileByNamesSimple

   import os

   # Prepare output file
   newFile = os.path.join(os.path.expanduser("~"), "output.cal")

   # Create test workspace. Normally just use reduced one
   GEM = LoadEmptyInstrument(Filename="GEM_Definition.xml")

   # Run the algorithm
   CreateCalFileByNames("GEM",newFile,"bank1,bank2,module1")


   # Check the output file
   print("File Exists: {}".format(os.path.exists(newFile)))

Output:

.. testoutput:: ExCreateCalFileByNamesSimple

   File Exists: True

.. testcleanup:: ExCreateCalFileByNamesSimple

   os.remove( newFile )



.. categories::

.. sourcelink::
