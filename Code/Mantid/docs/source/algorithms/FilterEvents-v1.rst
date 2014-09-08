.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm filters events from an
:ref:`EventWorkspace <EventWorkspace>` to one or multiple
:ref:`EventWorkspaces <EventWorkspace>` according to an input
`SplittersWorkspace <http://www.mantidproject.org/SplittersWorkspace>`_ containing a series of
splitters (i.e., `SplittingIntervals <http://www.mantidproject.org/SplittingInterval>`_).

Output
######

The output will be one or multiple workspaces according to the number of
index in splitters. The output workspace name is the combination of
parameter OutputWorkspaceBaseName and the index in splitter.

Calibration File
################

The calibration, or say correction, from the detector to sample must be
consider in fast log. Thus a calibration file is required. The math is

``TOF_calibrated = TOF_raw * correction(detector ID).``

The calibration is in column data format.

A reasonable approximation of the correction is

``correction(detector_ID) = L1/(L1+L2(detector_ID))``

Unfiltered Events
#################

Some events are not inside any splitters. They are put to a workspace
name ended with '\_unfiltered'.

If input property 'OutputWorkspaceIndexedFrom1' is set to True, then
this workspace shall not be outputed.

Difference from FilterByLogValue
################################

In FilterByLogValue(), EventList.splitByTime() is used.

In FilterEvents(), if FilterByPulse is selected true,
EventList.SplitByTime is called; otherwise, EventList.SplitByFullTime()
is called instead.

The difference between splitByTime and splitByFullTime is that
splitByTime filters events by pulse time, and splitByFullTime considers
both pulse time and TOF.

Therefore, FilterByLogValue is not suitable for fast log filtering.

Comparing with other event filtering algorithms
###############################################

Wiki page `EventFiltering <http://www.mantidproject.org/EventFiltering>`__ has a detailed
introduction on event filtering in MantidPlot.

.. categories::
