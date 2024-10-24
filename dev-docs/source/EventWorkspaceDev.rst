.. _Event Workspace Dev:

===========================
Event Workspace Development
===========================

.. contents::
  :local:

The following information will be useful to you if you want to write an
:ref:`algorithm` that is :ref:`Event Workspace <EventWorkspace>` aware.

Individual Neutron Event Data (TofEvent)
########################################

The `TofEvent <https://github.com/mantidproject/mantid/blob/main/Framework/Types/inc/MantidTypes/Event/TofEvent.h>`_ class holds information for each neutron detection event
data:

-  PulseTime: An absolute time of the pulse that generated this neutron.
   This is saved as an INT64 of the number of nanoseconds since Jan 1,
   1990; this can be converted to other date and time formats as needed.
   Internall the PulseTime is represented as a Kernel::DateAndTime type.
-  tof: Time-of-flight of the neutron, in microseconds, as a double.
   Note that this field can be converted to other units, e.g. d-spacing.

.. tip::
   There are in fact several variants of the Event type within Mantid. The common by far is the RAW TOF described above, but there are also ``Weighted`` events that offer better compression.

Lists of Events (EventList)
###########################

-  The EventList class consists of a list of TofEvent's. The order of
   this list is not significant, since various algorithms will resort by
   time of flight or pulse time, as needed.

-  Also contained in the EventList is a std::set of detector ID's. This tracks which detector(s) were hit by the events in the list. ``EventList`` is a subtype of ``ISpectrum``, which provides the interface to many of the spectrum level access methods.

-  The histogram bins (X axis) are also stored in EventList. The Y and E
   histogram data are not, however, as they are calculated on demand by the MRU
   (below).

The += operator can be used to append two EventList's together. The
lists of TofEvent's get appended, as is the list of
detector ID's. Don't mess with the udetmap manually if
you start appending event lists - just call
EventWorkpspace->makeSpectraMap to generate the spectra map (map between
spectrum # and detector IDs) by using the info in each EventList.

Most Recently Used List (MRUList)
#################################

An Event Workspace contains a list of the 100 most-recently used
histograms, a MRUList. This MRU caches the last histogram
data generated for fastest display.

A note about workspace index / spectrum number / detector ID
############################################################

For event workspaces there is no benefit, and only a drawback to grouping detectors in hardware, therefore most of the loading algorithms for event data **match** the workspace index and spectrum number
in the Event Workspace. Therefore, in an Event Workspace, the two numbers
will often be the same, and your workspace's Axis[1] is a simple 1:1 map. As
mentioned above, the detectorID is saved in EventList, but the
makeSpectraMap() method generates the usual SpectraDetectorMap object.

Workspace2D compatibility
#########################

Event Workspace is designed to be able to be read (but not written to)
like a :ref:`MatrixWorkspace <MatrixWorkspace>`. By default, if an algorithm
performs an operation and outputs a new workspace, the
WorkspaceFactory will create a :ref:`Workspace2D` *copy*
of your Event Workspace's histogram representation. If you attempt to
change an Event Workspace's Y or E data in place, you will get an ``NotImplementedError`` raised, since that is not possible.

A Note about Thread Safety
##########################

Thread safety can be surprising when using an Event Workspace:

If two threads *read* a Y histogram at the same time, this *can* cause
problems. This is because the histogramming code will try to sort the
event list. If two threads try to sort the same event list, you can get
segfaults.

Remember that the ``PARALLEL\_FOR1()``, ``PARALLEL\_FOR2()`` etc. macros will
perform the check Workspace->threadSafe() on the input Event Workspace.
This function will return *false* (thereby disabling parallelization) if
any of the event lists are unsorted.

You can go around this by forcing the parallel loop with a plain
``PARALLEL\_FOR()`` macro. **Make sure you do not read from the same
spectrum in parallel!**
