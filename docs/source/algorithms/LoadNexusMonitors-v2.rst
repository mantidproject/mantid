.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads all monitors found in a NeXus file into a single
:ref:`Workspace2D <Workspace2D>` (if there is no event data or if
``LoadOnly='Histogram'``) or into an :ref:`EventWorkspace
<EventWorkspace>` (if there is event monitor data or
``LoadOnly='Events'``).  The algorithm assumes that all of the
monitors are histograms and have the same bin boundaries. **NOTE:**
The entry is assumed to be in SNS or ISIS format, so the loader is
currently not generically applicable.

This version (v2) fixes a bug in the first version and now returns a
group workspace when invoked from Python with a multiperiod input
workspace. As a side-effect of the fix, the contained individual
workspaces for each of the periods are named slightly differently.

Event monitor and histogram monitor
###################################

There are two types of monitors, event monitors and histograms monitors.
Both of them are of class *NXmonitor* in NeXus file.

* Event monitor must contain all of the following three fields:

  * ``event_index``
  * ``event_time_offset``
  * ``event_time_zero``

* Histogram monitor must contain

  * ``data``
  * ``period_index``
  * ``time_of_flight``

Load NeXus file containing both event monitor and histogram monitor
###################################################################

For most files, the ``LoadOnly`` option can be set to an empty string
(the default) and the correct form of the data will be loaded
(i.e. events for event monitors and histograms for histogram
monitors. In some NeXus files, both histograms and events are
provided. For those cases, the user **must** use the ``LoadOnly``
option to specify what is desired.

ISIS event monitor
##################

ISIS monitor of event mode may contain entry ``data``.  In this case,
the monitor of event mode can be loaded in histogram mode.


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
