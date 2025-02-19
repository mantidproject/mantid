.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. figure:: /images/InstrumentTree.jpg
   :alt: Instrument Tree

   Instrument Tree

Create a dummy :ref:`calibration file <CalFile>` for diffraction focusing
with one group.

Offsets in the file are all sets to zero and all detectors are selected.
Overwrites a calibration file that already exists at the location
specified.

Detectors will be assigned to group one when using the
DiffractionFocussing algorithm.

Usage
-----
**Example - CreateDummyCalFile for MUSR**

.. include:: ../usagedata-note.txt

.. testcode:: ExCreateDummyCalFileSimple

   import os

   result = Load("MUSR00015189")
   group = result[0]
   ws_1 = group[0]
   ws_2 = group[1]

   newFile = os.path.join(os.path.expanduser("~"), "dummy.cal")

   # Run the Algorithm
   CreateDummyCalFile(ws_1,newFile)

   # Check the output file
   print("File Exists: {}".format(os.path.exists(newFile)))

   f = open( newFile, 'r' )
   file = f.read().split('\n')
   f.close()

   for line in file[0:6]:
       # print the line truncating before system dependent line break can take effect
       # also stripping off any trailing spaces
       print(line[0:89].rstrip())

Output:

.. testoutput:: ExCreateDummyCalFileSimple

   File Exists: True
   # Diffraction focusing calibration file created by Mantid
   # Detectors have been grouped using assembly names:MUSR
   # No template file, all offsets set to 0.0 and select to 1
   #  Number           UDET         offset      select  group
           0             33      0.0000000       1       1
           1             34      0.0000000       1       1

.. testcleanup:: ExCreateDummyCalFileSimple

  os.remove( newFile )


.. categories::

.. sourcelink::
