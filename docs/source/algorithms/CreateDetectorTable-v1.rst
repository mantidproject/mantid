.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a table workspace of detector information for a given matrix or peaks workspace.

Usage
-----

**Example**

.. testcode:: CreateDetectorTableExample

   ws = CreateSampleWorkspace()

   ws_detectors = CreateDetectorTable(ws)

   # Print the first row of the table
   print(ws_detectors.row(0))

Output:

.. testoutput:: CreateDetectorTableExample

  {'Index': 0.0, 'Phi': 0.0, 'Monitor': 'no', 'Spectrum No': 1, 'R': 5.0, 'Detector ID(s)': '100', 'Theta': 0.0}

.. categories::

.. sourcelink::
