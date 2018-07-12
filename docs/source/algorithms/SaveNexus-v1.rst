.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm SaveNexus will write a Nexus data file from the named
workspace. The file name can be an absolute or relative path and should
have the extension .nxs, .nx5 or .xml. Warning - using XML format can be
extremely slow for large data sets and generate very large files. Both
the extensions nxs and nx5 will generate HDF5 files.

The optional parameters can be used to control which spectra are saved
into the file (not yet implemented). If WorkspaceIndexMin and WorkspaceIndexMax
are given, then only that range to data will be saved.

A Mantid Nexus file may contain several workspace entries each labelled
with an integer starting at 1. If the file already contains n
workspaces, the new one will be labelled n+1.

In the future it may be possible to write other Nexus file types than
the one supported by SaveNexusProcessed.

Time series data
################

TimeSeriesProperty data within the workspace will be saved as NXlog
sections in the Nexus file. Only floating point logs are stored and
loaded at present.

Child Algorithms used
#####################

:ref:`algm-SaveNexusProcessed`

Usage
-----

.. testcode::

  import os
  # Create a file path in the user home directory
  filePath = os.path.expanduser('~/SavedNexusFile.nxs')

  # Create a workspace
  ws=CreateSampleWorkspace()
  # Save it in Nexus format
  SaveNexus(ws,filePath)

.. testcleanup::

  os.remove(filePath)

.. categories::

.. sourcelink::
