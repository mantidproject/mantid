.. _EventWorkspace:

===============
Event Workspace
===============

.. contents::
  :local:

What is it for?
______________________

Event Workspaces are specialised for time-of-flight neutron scattering. Event Workspaces are designed for sparse data storage of neutron events. Individual detector observations, including information about when that observation was made are stored as discrete items inside the workspace. The ability to keep more detailed information gives a number of advantages over coverting directly to a compressed form, such as allowing  more powerful filtering operations to be used.  

Summary for Users
-----------------------

The Event Workspace is a type of :ref:`Matrix Workspace <MatrixWorkspace>`,
where the information about each individual neutron detection event is
maintained. For you as a user, this means that:

-  There are many options for filtering an Event Workspace, such as :ref:`FilterByLogValue <algm-FilterByLogValue>`
-  You can histogram (via :ref:`rebin <algm-rebin>`) an Event Workspace over and over and no
   information is ever lost (as long as you choose the PreserveEvents option).
-  You can convert an Event Workspace to a histogram :ref:`Workspace 2D <Workspace2D>`
   by using the :ref:`Rebin <algm-Rebin>` algorithm.
-  You cannot modify the histogram Y values (for example, with the
   Divide algorithm) and keep the event data. If you use an algorithm
   that modifies the Y values, the output workspace will be a
   :ref:`Workspace 2D <Workspace2D>` using the current binning parameters.
-  Some algorithms are Event Workspace-aware, meaning that the output of
   it can be another Event Workspace. For example, the :ref:`Plus <algm-Plus>`
   algorithm will append the event lists if given two input
   Event Workspaces.


.. note:: If you set the same name on the output as the input of your algorithm, then you will overwrite the Event Workspace and lose that event-based information.

Working with Event Workspaces in Python
----------------------------------------

The python options for an Event Workspace are limitied - it is designed to be able to be read (but not written to)
like a :ref:`MatrixWorkspace <MatrixWorkspace>`.  You can look at the :ref:`Event Workspace API reference <mantid.api.IEventWorkspace>` for a full list of properties and operations, but here are some of the key ones.

Accessing Workspaces
####################

The methods for getting a variable to an Event Workspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is an Event Workspace you can use this:

.. testcode:: CheckEventWorkspace

    from mantid.simpleapi import *
    from mantid.api import IEventWorkspace

    eventWS = CreateSampleWorkspace(WorkspaceType="Event")

    if isinstance(eventWS, IEventWorkspace):
        print(eventWS.name() + " is an " + eventWS.id())

Output:

.. testoutput:: CheckEventWorkspace
    :options: +NORMALIZE_WHITESPACE

    eventWS is an EventWorkspace


Event Workspace Properties
###########################

In addition to the Properties of the :ref:`MatrixWorkspace <MatrixWorkspace>`, the Event Workspace also has the following:

.. testcode:: EventWorkspaceProperties

   from mantid.simpleapi import *
   eventWS = CreateSampleWorkspace(WorkspaceType="Event")

   print("Number of events: {}".format(eventWS.getNumberEvents()))
   print("Maximum time of flight: {}".format(eventWS.getTofMax()))

.. testoutput:: EventWorkspaceProperties
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   Number of events: ...
   Maximum time of flight: ...

Event lists
###########

Event Workspaces store their data in event lists, one per spectrum.  You can access them using:

.. testcode:: EventWorkspaceEventLists

   from mantid.simpleapi import *
   eventWS = CreateSampleWorkspace(WorkspaceType="Event")

   # get the number of event lists
   evListCount = eventWS.getNumberHistograms()

   # Get the first event list
   evList = eventWS.getSpectrum(0)

   # Get some basic information
   print("Number of events in event List 0: {}".format(evList.getNumberEvents()))
   print("Minimum time of flight in event List 0: {}".format(evList.getTofMax()))
   print("Maximum time of flight in event List 0: {}".format(evList.getTofMax()))
   print("Memory used: {}".format(evList.getMemorySize()))
   print("Type of Events: {}".format(evList.getEventType()))

   # Get a vector of the pulse times of the events
   pulseTimes = evList.getPulseTimes()

   # Get a vector of the TOFs of the events
   tofs = evList.getTofs()

   # Get a vector of the weights of the events
   weights = evList.getWeights()

   # Get a vector of the errors squared of the weights of the events
   weightErrors = evList.getWeightErrors()

   # Integrate the events between  a range of X values
   print("Events between 1000 and 5000: {}".format(evList.integrate(1000,5000,False)))

   #Check if the list is sorted in TOF
   print("Is sorted by TOF: {}".format(evList.isSortedByTof()))

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

   from mantid.simpleapi import *
   import math
   eventWS = CreateSampleWorkspace(WorkspaceType="Event")
   # Get the first event list
   evList = eventWS.getSpectrum(0)

   # Add an offset to the pulsetime (wall-clock time) of each event in the list.
   print("First pulse time before addPulsetime: {}".format(evList.getPulseTimes()[0]))
   seconds = 200.0
   evList.addPulsetime(seconds)
   print("First pulse time after addPulsetime: {}".format(evList.getPulseTimes()[0]))

   # Add an offset to the TOF of each event in the list.
   print("First tof before addTof: {}".format(evList.getTofs()[0]))
   microseconds = 2.7
   evList.addTof(microseconds)
   print("First tof after addTof: {}".format(evList.getTofs()[0]))

   # Convert the tof units by scaling by a multiplier.
   print("First tof before scaleTof: {}".format(evList.getTofs()[0]))
   factor = 1.5
   evList.scaleTof(factor)
   print("First tof after scaleTof: {}".format(evList.getTofs()[0]))

   # Multiply the weights in this event list by a scalar with an error.
   print("First event weight before multiply: {0} +/- {1}".format(evList.getWeights()[0], math.sqrt(evList.getWeightErrors()[0])))
   factor = 10.0
   error = 5.0
   evList.multiply(factor,error)
   print("First event weight after multiply: {0} +/- {1}".format(evList.getWeights()[0], math.sqrt(evList.getWeightErrors()[0])))

   # Divide the weights in this event list by a scalar with an error.
   print("First event weight before divide: {0} +/- {1}".format(evList.getWeights()[0], math.sqrt(evList.getWeightErrors()[0])))
   factor = 1.5
   error = 0.0
   evList.divide(factor,error)
   print("First event weight after divide: {0} +/- {1}".format(evList.getWeights()[0], math.sqrt(evList.getWeightErrors()[0])))

   # Mask out events that have a tof between tofMin and tofMax (inclusively)
   print("Number of events before masking: {}".format(evList.getNumberEvents()))
   evList.maskTof(1000,5000)
   print("Number of events after masking: {}".format(evList.getNumberEvents()))

.. testoutput:: ChangingEventLists
   :hide:
   :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

   First pulse time before addPulsetime: 2010-01-01T00:3...
   First pulse time after addPulsetime: 2010-01-01T00:3...
   First tof before addTof: 1...
   First tof after addTof: 1...
   First tof before scaleTof: 1...
   First tof after scaleTof: 2...
   First event weight before multiply: 1.0... +/- 1.0...
   First event weight after multiply: 10.0 +/- 3.34...
   First event weight before divide: 10.0 +/- 3.34...
   First event weight after divide: 6.6... +/- 2.73...
   Number of events before masking: ...
   Number of events after masking: ...

For Developers/Writing Algorithms
---------------------------------

See the Event Workspace section in development `documentation <http://developer.mantidproject.org/>`_

.. categories:: Concepts
