.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a mapping file to be used with the Group Detectors algorithm with a preset number of groups over a range of spectra.

Usage
-----

**Example - Create mapping file for IRIS**

.. testcode:: exCreateIRISMapFile

   import os

   map_filename = 'iris.map'
   num_groups = 20
   spectra_range = [3, 53]

   CreateMappingFile(map_filename, num_groups, spectra_range)

   print "File Exists:", os.path.exists(map_filename)
  
**Output:**

.. testoutput:: exCreateIRISMapFile

   File Exists: True

.. categories::
