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

This version (v1) of this algorithm has a bug where, in case of a
multiperiod input workspace, invoking this Algorithm from Python returns
a tuple that contains the resulting group workspace as the first element,
followed by references to the individual workspaces as siblings (in
addition to being contained within the group).

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
