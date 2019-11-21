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
   first_row = ws_detectors.row(0)
   print("Index: {0}, Spectrum No: {1}, Detector ID: {2}, R: {3}, Theta: {4}, Phi: {5}, Monitor: {6}".format(
         first_row['Index'],
         first_row['Spectrum No'],
         first_row['Detector ID(s)'],
         first_row['R'],
         first_row['Theta'],
         first_row['Phi'],
         first_row['Monitor']))

Output:

.. testoutput:: CreateDetectorTableExample

   Index: 0.0, Spectrum No: 1, Detector ID: 100, R: 5.0, Theta: 0.0, Phi: 0.0, Monitor: no
.. categories::

.. sourcelink::
