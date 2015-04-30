.. _EventWorkspace:

Event Workspace
===============

Quick Summary For Users
-----------------------

The EventWorkspace is a type of :ref:`MatrixWorkspace <MatrixWorkspace>`,
where the information about each individual neutron detection event is
maintained. For you as a user, this means that:

-  You can :ref:`rebin <algm-rebin>` an EventWorkspace over and over and no
   information is ever lost.
-  The histogram (Y and E values) of an EventWorkspace are only
   calculated when they are requested.

   -  You typically get better performance, even for very fine binning.

-  You can convert an EventWorkspace to a :ref:`Workspace2D <Workspace2D>`
   by using the :ref:`Rebin <algm-Rebin>` algorithm and changing the output
   workspace name.
-  You cannot modify the histogram Y values (for example, with the
   Divide algorithm) and keep the event data. If you use an algorithm
   that modifies the Y values, the output workspace will be a
   :ref:`Workspace2D <Workspace2D>` using the current binning parameters.
   If you set the same name on the output as the input of your
   algorithm, then you will overwrite the EventWorkspace and lose that
   event-based information.
-  Some algorithms are EventWorkspace-aware, meaning that the output of
   it can be another EventWorkspace. For example, the :ref:`Plus <algm-Plus>`
   algorithm will append the event lists if given two input
   EventWorkspaces.
-  Since it retains the most information, it is advantageous to keep
   your data as an EventWorkspace for as much processing as is possible
   (as long as you have enough memory!).

For Developers/Writing Algorithms
---------------------------------

The following information will be useful to you if you want to write an
algorithm that is EventWorkspace-aware.

Individual Neutron Event Data (TofEvent)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The TofEvent class holds information for each neutron detection event
data:

-  PulseTime: An absolute time of the pulse that generated this neutron.
   This is saved as an INT64 of the number of nanoseconds since Jan 1,
   1990; this can be converted to other date and time formats as needed.
-  tof: Time-of-flight of the neutron, in microseconds, as a double.
   Note that this field can be converted to other units, e.g. d-spacing.

Lists of Events (EventList)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  The EventList class consists of a list of TofEvent's. The order of
   this list is not significant, since various algorithms will resort by
   time of flight or pulse time, as needed.

-  Also contained in the EventList is a std::set of detector ID's. This
   tracks which detector(s) were hit by the events in
   the list.

-  The histogram bins (X axis) are also stored in EventList. The Y and E
   histogram data are not, however, as they are calculated by the MRU
   (below).

The += operator can be used to append two EventList's together. The
lists of TofEvent's get appended, as is the list of
detector ID's. Don't mess with the udetmap manually if
you start appending event lists - just call
EventWorkpspace->makeSpectraMap to generate the spectra map (map between
spectrum # and detector IDs) by using the info in each EventList.

Most Recently Used List (MRUList)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

An EventWorkspace contains a list of the 100 most-recently used
histograms, a MRUList. This MRU caches the last histogram
data generated for fastest display.

A note about workspace index / spectrum number / detector ID
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The loading algorithms **match** the workspace index and spectrum number
in the EventWorkspace. Therefore, in an EventWorkspace, the two numbers
will be the same, and your workspace's Axis[1] is a simple 1:1 map. As
mentioned above, the detectorID is saved in EventList, but the
makeSpectraMap() method generates the usual SpectraDetectorMap object.

Workspace2D compatibility
~~~~~~~~~~~~~~~~~~~~~~~~~

EventWorkspace is designed to be able to be read (but not written to)
like a :ref:`Workspace2D <Workspace2D>`. By default, if an algorithm
performs an operation and outputs a new workspace, the
WorkspaceFactory will create a Workspace2D *copy*
of your EventWorkspace's histogram representation. If you attempt to
change an EventWorkspace's Y or E data in place, you will get an error
message, since that is not possible.

A Note about Thread Safety
~~~~~~~~~~~~~~~~~~~~~~~~~~

Thread safety can be surprising when using an EventWorkspace:

If two threads *read* a Y histogram at the same time, this *can* cause
problems. This is because the histogramming code will try to sort the
event list. If two threads try to sort the same event list, you can get
segfaults.

Remember that the PARALLEL\_FOR1(), PARALLEL\_FOR2() etc. macros will
perform the check Workspace->threadSafe() on the input EventWorkspace.
This function will return *false* (thereby disabling parallelization) if
any of the event lists are unsorted.

You can go around this by forcing the parallel loop with a plain
PARALLEL\_FOR() macro. **Make sure you do not read from the same
spectrum in parallel!**



.. categories:: Concepts