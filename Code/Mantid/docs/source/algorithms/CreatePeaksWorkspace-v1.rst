.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Create an empty `PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_. Use
:ref:`algm-LoadIsawPeaks` or :ref:`algm-FindPeaksMD` to
create a peaks workspace with peaks.

This workspace can serve as a starting point for modifying the
`PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_, using the GUI or python scripting,
for example.

Usage
-----

**Example: An empty table, not tied to an instrument**

.. testcode:: ExEmptyTable

    ws = CreatePeaksWorkspace()
    print "Created a %s with %i rows" % (ws.id(), ws.rowCount())

Output:

.. testoutput:: ExEmptyTable

    Created a PeaksWorkspace with 0 rows

**Example: With a few peaks in place**

.. testcode:: ExTableWithRows

    sampleWs = CreateSampleWorkspace()
    ws = CreatePeaksWorkspace(InstrumentWorkspace=sampleWs,NumberOfPeaks=3)
    print "Created a %s with %i rows" % (ws.id(), ws.rowCount())

Output:

.. testoutput:: ExTableWithRows

    Created a PeaksWorkspace with 3 rows

.. categories::
