.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is useful for Powder Diffraction Reduction. In practice, when we have data collected
for samples at various temperature points, we don't want to repeat the measurement for the empty
background at all the temperature points. Instead, we want to measure the empty container background
at selected critical temperature points and for the rest of points, we want to do perform the interpolation
to estimate the empty container.

This algorithm will perform a linear interpolation on the background temperature for two empty container runs.
Below is the function used for the interpolation:

.. math::

    factor1 = (temp_{interpo} - temp_{low}) / (temp_{high} - temp_{low})
    factor2 = (temp_{high} - temp_{interpo}) / (temp_{high} - temp_{low})
    WS_{interpolated} = WS_{high} * factor1 + WS_{low} * factor2

where :math:`WS_{low}` and :math:`WS_{high}` are the workspaces containing the run data, :math:`temp_{low}` and
:math:`temp_{high}` are the two extreme temperatures the empty containers were measured at, and
:math:`temp_{interpo}` is the target temperature to interpolate at.





Usage
-----

**Example - Background Interpolation:**

.. testcode::

   # create workspace
   dataX = [0,1,2,3,4,5,6,7,8,9] # or use dataX=range(0,10)
   dataY1 = [1,1,1,1,1,1,1,1,1,1] # or use dataY=[1]*10
   dataY2 = [2,2,2,2,2,2,2,2,2,2] # or use dataY=[2]*10
   ws1 = CreateWorkspace(dataX, dataY1)
   ws2 = CreateWorkspace(dataX, dataY2)
   wsGroup = GroupWorkspaces("ws1,ws2")
   interpoTemp = "200"
   # CreateWorkspace does not add the "SampleTemp" property so we need to add it here
   ws1.getRun().addProperty("SampleTemp", "100", False)
   ws2.getRun().addProperty("SampleTemp", "300", False)

   # Perform the background interpolation
   outputWS = InterpolateBackground(wsGroup, interpoTemp)

   # Check output
   print("Interpolated Y values are: {}".format(outputWS.readY(0)))

Output:

.. testoutput::

   Interpolated Y values are: [1.5 1.5 1.5 1.5 1.5 1.5 1.5 1.5 1.5 1.5]


.. categories::

.. sourcelink::
