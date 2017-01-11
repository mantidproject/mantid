.. _EventWorkspace:

===============
Event Workspace
===============

.. contents::
  :local:

Quick Summary For Users
-----------------------

The EventWorkspace is a type of :ref:`MatrixWorkspace <MatrixWorkspace>`,
where the information about each individual neutron detection event is
maintained. For you as a user, this means that:

-  You can :ref:`rebin <algm-rebin>` an EventWorkspace over and over and no
   information is ever lost (as long as you choose the PreserveEvents option).
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

Working with Event Workspaces in Python
----------------------------------------

EventWorkspace is designed to be able to be read (but not written to)
like a :ref:`MatrixWorkspace <MatrixWorkspace>`.  You can look at the :ref:`Event Workspace API reference <mantid.api.IEventWorkspace>` for a full list of properties and operations, but here are some of the key ones.

Accessing Workspaces
####################

The methods for getting a variable to an EventWorkspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is an Event Workspace you can use this:

.. testcode:: CheckEventWorkspace

    from mantid.api import IEventWorkspace

    eventWS = CreateSampleWorkspace(WorkspaceType="Event")

    if isinstance(eventWS, IEventWorkspace):
        print eventWS.name() + " is an " + eventWS.id()

Output:

.. testoutput:: CheckEventWorkspace
    :options: +NORMALIZE_WHITESPACE

    eventWS is an EventWorkspace


Event Workspace Properties
###########################

In addition to the Properties of the :ref:`MatrixWorkspace <MatrixWorkspace>`, the Event Workspace also has the following:

.. testcode:: EventWorkspaceProperties

   eventWS = CreateSampleWorkspace(WorkspaceType="Event")

   print "Number of events:", eventWS.getNumberEvents()
   print "Maximum time of flight:", eventWS.getTofMax()

.. testoutput:: EventWorkspaceProperties
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   Number of events: ...
   Maximum time of flight: ...

Event lists
###########

Event Workspaces store their data in event lists, one per spectrum.  You can access them using:

.. testcode:: EventWorkspaceEventLists

   eventWS = CreateSampleWorkspace(WorkspaceType="Event")

   # get the number of event lists
   evListCount = eventWS.getNumberHistograms()

   # Get the first event list
   evList = eventWS.getSpectrum(0)

   # Get some basic information
   print "Number of events in event List 0:", evList.getNumberEvents()
   print "Minimum time of flight in event List 0:", evList.getTofMax()
   print "Maximum time of flight in event List 0:", evList.getTofMax()
   print "Memory used:", evList.getMemorySize()
   print "Type of Events:", evList.getEventType()

   # Get a vector of the pulse times of the events
   pulseTimes = evList.getPulseTimes()

   # Get a vector of the TOFs of the events
   tofs = evList.getTofs()

   # Get a vector of the weights of the events
   weights = evList.getWeights()

   # Get a vector of the errors squared of the weights of the events
   weightErrors = evList.getWeightErrors()

   # Integrate the events between  a range of X values
   print "Events between 1000 and 5000:", evList.integrate(1000,5000,False)

   #Check if the list is sorted in TOF
   print "Is sorted by TOF:", evList.isSortedByTof()

.. testoutput:: EventWorkspaceEventLists
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   Number of events in event List 0: ...
   Minimum time of flight in event List 0: ...
   Maximum time of flight in event List 0: ...
   Memory used: ...
   Type of Events: TOF
   Events between 1000 and 5000: ...
   Is sorted by TOF: True

Changing EventLists
^^^^^^^^^^^^^^^^^^^

Please note these should only be done as part of a Python Algorithm, otherwise these actions will not be recorded in the workspace history.


.. testcode:: ChangingEventLists

   import math
   eventWS = CreateSampleWorkspace(WorkspaceType="Event")
   # Get the first event list
   evList = eventWS.getSpectrum(0)

   # Add an offset to the pulsetime (wall-clock time) of each event in the list.
   print "First pulse time before addPulsetime:", evList.getPulseTimes()[0]
   seconds = 200.0
   evList.addPulsetime(seconds)
   print "First pulse time after addPulsetime:", evList.getPulseTimes()[0]

   # Add an offset to the TOF of each event in the list.
   print "First tof before addTof:", evList.getTofs()[0]
   microseconds = 2.7
   evList.addTof(microseconds)
   print "First tof after addTof:", evList.getTofs()[0]

   # Convert the tof units by scaling by a multiplier.
   print "First tof before scaleTof:", evList.getTofs()[0]
   factor = 1.5
   evList.scaleTof(factor)
   print "First tof after scaleTof:", evList.getTofs()[0]

   # Multiply the weights in this event list by a scalar with an error.
   print "First event weight before multiply:", evList.getWeights()[0], \
         "+/-", math.sqrt(evList.getWeightErrors()[0])
   factor = 10.0
   error = 5.0
   evList.multiply(factor,error)
   print "First event weight after multiply:", evList.getWeights()[0], \
         "+/-", math.sqrt(evList.getWeightErrors()[0])

   # Divide the weights in this event list by a scalar with an error.
   print "First event weight before divide:", evList.getWeights()[0], \
         "+/-", math.sqrt(evList.getWeightErrors()[0])
   factor = 1.5
   error = 0.0
   evList.divide(factor,error)
   print "First event weight after divide:", evList.getWeights()[0], \
         "+/-", math.sqrt(evList.getWeightErrors()[0])

   # Mask out events that have a tof between tofMin and tofMax (inclusively)
   print "Number of events before masking:", evList.getNumberEvents()
   evList.maskTof(1000,5000)
   print "Number of events after masking:", evList.getNumberEvents()

.. testoutput:: ChangingEventLists
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   First pulse time before addPulsetime: 2010-01-01T00:32:55...
   First pulse time after addPulsetime: 2010-01-01T00:36:15...
   First tof before addTof: 118...
   First tof after addTof: 121...
   First tof before scaleTof: 121...
   First tof after scaleTof: 181...
   First event weight before multiply: 1.0... +/- 1.0...
   First event weight after multiply: 10.0 +/- 3.34...
   First event weight before divide: 10.0 +/- 3.34...
   First event weight after divide: 6.6... +/- 2.73...
   Number of events before masking: ...
   Number of events after masking: ...

For Developers/Writing Algorithms
---------------------------------

The following information will be useful to you if you want to write an
algorithm that is EventWorkspace-aware.

Individual Neutron Event Data (TofEvent)
########################################

The TofEvent class holds information for each neutron detection event
data:

-  PulseTime: An absolute time of the pulse that generated this neutron.
   This is saved as an INT64 of the number of nanoseconds since Jan 1,
   1990; this can be converted to other date and time formats as needed.
-  tof: Time-of-flight of the neutron, in microseconds, as a double.
   Note that this field can be converted to other units, e.g. d-spacing.

Lists of Events (EventList)
###########################

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
#################################

An EventWorkspace contains a list of the 100 most-recently used
histograms, a MRUList. This MRU caches the last histogram
data generated for fastest display.

A note about workspace index / spectrum number / detector ID
############################################################

For event workspaces there is no benefit, and only a drawback to grouping detectors in hardware, therefore most of the loading algorithms for event data **match** the workspace index and spectrum number
in the EventWorkspace. Therefore, in an EventWorkspace, the two numbers
will be the same, and your workspace's Axis[1] is a simple 1:1 map. As
mentioned above, the detectorID is saved in EventList, but the
makeSpectraMap() method generates the usual SpectraDetectorMap object.

Workspace2D compatibility
#########################

EventWorkspace is designed to be able to be read (but not written to)
like a :ref:`MatrixWorkspace <MatrixWorkspace>`. By default, if an algorithm
performs an operation and outputs a new workspace, the
WorkspaceFactory will create a :ref:`Workspace2D` *copy*
of your EventWorkspace's histogram representation. If you attempt to
change an EventWorkspace's Y or E data in place, you will get an error
message, since that is not possible.

A Note about Thread Safety
##########################

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

.. include:: WorkspaceNavigation.txt



.. categories:: Concepts
