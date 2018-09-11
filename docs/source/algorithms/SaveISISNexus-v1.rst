.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm SaveISISNexus will write a Nexus data file from the given RAW file.
The output file name can be an absolute or relative path and should
have the extension .nxs, .nx5 or .xml. Warning - using XML format can be
extremely slow for large data sets and generate very large files. Both
the extensions nxs and nx5 will generate HDF5 files.

Usage
-----

.. testcode::

  import os
  # Create a file path in the user home directory
  filePath = os.path.expanduser('~/SavedISISNexusFile.nxs')

  # Save a RAW file in Nexus format
  SaveISISNexus('IRS26173.raw',filePath)

.. testcleanup::

  os.remove(filePath)
  
.. categories::

.. sourcelink::
