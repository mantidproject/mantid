.. _EventFiltering:

===============
Event Filtering
===============

.. contents::
   :local:

In MantidPlot, there are a few algorithms working with event
filtering.  These algorithms are :ref:`algm-FilterByTime`,
:ref:`algm-FilterByLogValue`, :ref:`algm-FilterEvents`, and
:ref:`algm-GenerateEventsFilter`.

How to generate event filters
=============================

Generating filters explicitly
-----------------------------

:ref:`algm-FilterEvents` reads and parses a
:class:`mantid.api.ISplittersWorkspace` object to generate a list of
:ref:`SplittingIntervals <SplittingInterval>`, which are used to split
neutron events to specified output workspaces according to the times
that they arrive detectors.

There can be two approaches to create a
:class:`mantid.api.ISplittersWorkspace`.

* :ref:`algm-GenerateEventsFilter` generate event filters by either by
  time or log value.  The output filters are stored in a
  :ref:`SplittersWorkspace`, which is taken as an input property of
  :ref:`algm-FilterEvents`.

* Users can create a :class:`mantid.api.ISplittersWorkspace` from scrach from Python
  script, because :class:`mantid.api.ISplittersWorkspace` inherits from
  :ref:`TableWorkspace <Table Workspaces>`.

Generating inexplicit filters
-----------------------------

:ref:`algm-FilterByTime` and :ref:`algm-FilterByLogValue` generate event filters during execution.

* :ref:`algm-FilterByTime` generates a set of :ref:`SplittingInterval`
  according to user-specified setup for time splicing;

* :ref:`algm-FilterByLogValue` generates a set of
  :ref:`SplittingInterval` according to the value of a specific sample
  log.

:ref:`algm-GenerateEventsFilter` and :ref:`algm-FilterEvents` vs :ref:`algm-FilterByTime` and :ref:`algm-FilterByLogValue`
--------------------------------------------------------------------------------------------------------------------------

* If :ref:`algm-GenerateEventsFilter` and :ref:`algm-FilterEvents` are
  set up correctly, they can have the same functionality as
  :ref:`algm-FilterByTime` and :ref:`algm-FilterByLogValue`.

* :ref:`algm-FilterEvents` is able to filter neutron events by either
  their pulse times or their absolute times.  An neutron event's
  abolute time is the summation of its pulse time and TOF.

* :ref:`algm-FilterByLogValue` and :ref:`algm-FilterByTime` can only
  split neutron events by their pulse time.

Types of events filters
=======================

Filtering by :ref:`SplittingInterval`
-------------------------------------

:ref:`SplittingInterval` is an individual class to indicate an
independent time splitter.  Any event can be filtered by a
:ref:`SplittingInterval` object.

:ref:`SplittersWorkspace` is a :ref:`TableWorkspace <Table
Workspaces>` that stors a set of :ref:`SplittingInterval`.

Filtering by duplicate entries/booleans
---------------------------------------

Duplicate entries in a :ref:`TimeSeriesProperty` and boolean type of
:ref:`TimeSeriesProperty` are used in MantidPlot too to serve as time
splitters.

These two are applied in the MantidPlot log viewing functionality and
unfortunately intrudes into :ref:`TimeSeriesProperty`.

As time splitters are better to be isolated from logs, which are
recorded in :ref:`TimeSeriesProperty`, it is not
recommended to set up event filters by this approach.
