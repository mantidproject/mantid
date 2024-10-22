
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given an input workspace containing data with a background and a table of
user-selected points defining the background, creates a new workspace
containing background data that can be subtracted from the original data.

The background is constructed using linear interpolation at the same X values as the input workspace.

Usage
-----

**Example - CreateUserDefinedBackground**

.. testcode:: CreateUserDefinedBackgroundExample

   # Create data: background + peak
   dataX = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
   background = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45]
   peak = [0, 1, 4, 1, 0, 0, 0, 0, 0, 0]
   dataY = [a + b for a, b in zip(background, peak)]
   dataWS = CreateWorkspace(dataX, dataY)

   # Create table of user-supplied background points
   # In real use this would be done via the GUI
   background_points = CreateEmptyTableWorkspace()
   background_points.addColumn("double", "X", 1)
   background_points.addColumn("double", "Y", 2)
   for i in range(10):
      background_points.addRow([dataX[i], background[i]])

   # Create background workspace and subtract it from data
   backgroundWS = CreateUserDefinedBackground(InputWorkspace=dataWS, BackgroundPoints=background_points)
   peakWS = Minus(dataWS, backgroundWS)


   # Check that workspace matches expected peak
   expected = CreateWorkspace(dataX, peak)
   result, messages = CompareWorkspaces(peakWS, expected)
   print("Workspaces match: {}".format(result))


Output:

.. testoutput:: CreateUserDefinedBackgroundExample

   Workspaces match: True

.. categories::

.. sourcelink::
