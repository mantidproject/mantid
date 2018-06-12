.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads all monitors found in a NeXus file into a single
:ref:`Workspace2D <Workspace2D>` (if there is no event data or if 
MonitorsAsEvents is false) or into an 
:ref:`EventWorkspace <EventWorkspace>` (if event monitor data is
found).  The algorithm assumes that all of the monitors are histograms
and have the same bin boundaries. **NOTE:** The entry is assumed to be
in SNS or ISIS format, so the loader is currently not generically
applicable.

This version (v2) fixes a bug in the first version and now returns a
group workspace when invoked from Python with a multiperiod input
workspace. As a side-effect of the fix, the contained individual
workspaces for each of the periods are named slightly differently.

Event monitor and histogram monitor
###################################

There are two types of monitors, event monitors and histograms monitors.
Both of them are of class *NXmonitor* in NeXus file.

 * Event monitor must contain all of the following three entries:
   * event_index
   * event_time_offset
   * event_time_zero

 * Histogram monitor must contain entry
   * data
   * period_index

ISIS event monitor
##################

ISIS monitor of event mode may contain entry *data*.  In this case,
the monitor of event mode can be loaded in histogram mode.


Load NeXus file containing both event monitor and histogram monitor
###################################################################

There are a few use cases to load monitor's data if the NeXus file has coexisting
event monitors and histogram monitors.

 1. Load all monitors as histogram.
    * requirement: all event monitors have entry *data*;
    * Set *MonitorsAsEvents* to False, *LoadEventMonitor* to True and *LoadHistoMonitor* to True;

 2. Load monitors in event mode only.
    * Set *MonitorsAsEvents* to True, *LoadEventMonitor* to True and *LoadHistoMonitor* to False;

 3. Load monitors in histogram mode only.
    * Set *MonitorsAsEvents* to False, *LoadEventMonitor* to False and *LoadHistoMonitor* to True.


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadNexusMonitors("CNCS_7860_event.nxs")
    # CNCS has 3 monitors
    print("Number of monitors = {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: Ex

    Number of monitors = 3

.. categories::

.. sourcelink::
