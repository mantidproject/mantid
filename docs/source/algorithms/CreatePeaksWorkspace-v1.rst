.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to create a:

- :ref:`PeaksWorkspace <PeaksWorkspace>` (Default, or when `OutputType` is set to `Peak`.)
- :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>` (when `OutputType` is set to `LeanElasticPeak`)

Use :ref:`algm-LoadIsawPeaks` or :ref:`algm-FindPeaksMD` to
create a peaks workspace with peaks.

This workspace can serve as a starting point for modifying the
:ref:`PeaksWorkspace <PeaksWorkspace>`, using the GUI or python scripting,
for example.

If the input workspace is a MDWorkspace then the instrument from the
first experiment info is used.

Usage
-----

**Example: An empty table, not tied to an instrument**

.. testcode:: ExEmptyPeaksworkspaceTable

    ws = CreatePeaksWorkspace()
    print("Created a {} with {} rows".format(ws.id(), ws.rowCount()))

Output:

.. testoutput:: ExEmptyPeaksworkspaceTable

    Created a PeaksWorkspace with 0 rows

**Example: Create an empty LeanElasticPeaksWorkspace, not tied to an instrument**

.. testcode:: ExEmptyLeanElasticPeaksworkspaceTable

    ws = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")
    print("Created a {} with {} rows".format(ws.id(), ws.rowCount()))

Output:

.. testoutput:: ExEmptyLeanElasticPeaksworkspaceTable

    Created a LeanElasticPeaksWorkspace with 0 rows

**Example: With a few peaks in place**

.. testcode:: ExTableWithRows

    sampleWs = CreateSampleWorkspace()
    ws = CreatePeaksWorkspace(InstrumentWorkspace=sampleWs,NumberOfPeaks=3)
    print("Created a {} with {} rows".format(ws.id(), ws.rowCount()))

Output:

.. testoutput:: ExTableWithRows

    Created a PeaksWorkspace with 3 rows

.. categories::

.. sourcelink::
