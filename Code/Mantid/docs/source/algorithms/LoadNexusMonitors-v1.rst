.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads all monitors found in a NeXus file into a single
:ref:`Workspace2D <Workspace2D>`. The algorithm
assumes that all of the
monitors are histograms and have the same bin boundaries. **NOTE:** The
entry is assumed to be in SNS format, so the loader is currently not
generically applicable. It is also written for single entry files and
will need tweaking to handle period data where the monitors are
different.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = LoadNexusMonitors("CNCS_7860_event.nxs")
    # CNCS has 3 monitors
    print "Number of monitors =", ws.getNumberHistograms()

Output:

.. testoutput:: Ex

    Number of monitors = 3

.. categories::
