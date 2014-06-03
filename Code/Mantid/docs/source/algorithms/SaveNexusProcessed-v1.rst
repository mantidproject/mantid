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

This algorithm will save `EventWorkspaces <EventWorkspace>`__ with full
event data, unless you uncheck *PreserveEvents*, in which case the
histogram version of the workspace is saved.

Optionally, you can check *CompressNexus*, which will compress the event
data. **Warning!** This can be *very* slow, and only gives approx. 40%
compression because event data is typically denser than histogram data.
*CompressNexus* is off by default.

.. categories::
