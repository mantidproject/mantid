.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm SaveNexusProcessed will write a Nexus data file from the
named workspace. This can later be loaded using
:ref:`algm-LoadNexusProcessed`.

The file name can be an absolute or relative path and should have the
extension .nxs, .nx5 or .xml. Warning - using XML format can be
extremely slow for large data sets and generate very large files. Both
the extensions nxs and nx5 will generate HDF5 files.

The optional parameters can be used to control which spectra are saved
into the file. If WorkspaceIndexMin and WorkspaceIndexMax are given,
then only that range to data will be loaded.

A Mantid Nexus file may contain several workspace entries each labelled
with an integer starting at 1. If the file already contains n
workspaces, the new one will be labelled n+1.

Time series data
################

TimeSeriesProperty data within the workspace will be saved as NXlog
sections in the Nexus file. Only floating point logs are stored and
loaded at present.

EventWorkspaces
###############

This algorithm will save :ref:`EventWorkspaces <EventWorkspace>` with full
event data, unless you uncheck *PreserveEvents*, in which case the
histogram version of the workspace is saved.

Optionally, you can check *CompressNexus*, which will compress the event
data. **Warning!** This can be *very* slow, and only gives approx. 40%
compression because event data is typically denser than histogram data.
*CompressNexus* is off by default.

Usage
-----
**Example - a basic example using SaveNexusProcessed.**

.. testcode:: ExSaveNexusProcessedSimple

    import os

    ws = CreateSampleWorkspace()
    file_name = "myworkspace.nxs"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveNexusProcessed(ws, path)

    print os.path.isfile(path)

Output:

.. testoutput:: ExSaveNexusProcessedSimple

    True

.. testcleanup:: ExSaveNexusProcessedSimple

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])


**Example - an example using SaveNexusProcessed with additonal options.**

.. testcode:: ExSaveNexusProcessedOptions

    import os

    ws = CreateSampleWorkspace()
    file_name = "myworkspace.nxs"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveNexusProcessed(ws, path, Title="MyWorkspace", WorkspaceIndexMin=0, WorkspaceIndexMax=9)

    print os.path.isfile(path)

    ws = Load(path)
    print "Saved workspace has %d spectra" % ws.getNumberHistograms()

Output:

.. testoutput:: ExSaveNexusProcessedOptions

    True
    Saved workspace has 10 spectra

.. testcleanup:: ExSaveNexusProcessedOptions

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])

**Example - an example using SaveNexusProcessed to save an Event workspace.**

.. testcode:: ExSaveNexusProcessedEvent

    import os

    ws = CreateSampleWorkspace("Event")
    file_name = "myworkspace.nxs"
    path = os.path.join(os.path.expanduser("~"), file_name)
    SaveNexusProcessed(ws, path, CompressNexus=True, PreserveEvents=True)

    print os.path.isfile(path)

    ws = Load(path)
    print "Saved workspace has %d spectra" % ws.getNumberHistograms()
    
Output:

.. testoutput:: ExSaveNexusProcessedEvent

    True
    Saved workspace has 200 spectra

.. testcleanup:: ExSaveNexusProcessedEvent

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])


.. categories::
