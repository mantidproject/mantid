.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs interpolation of points onto a cubic spline. The
algorithm takes two input workspaces: one that is used to define the
spline and one that contains a number of spectra to be interpolated onto
the spline. Values smaller or larger than the interpolation range will
be set to the first and last value of the WorkspaceToInterpolate,
respectively. This applies only for the OutputWorkspace and for example
not for the workspace of derivatives, where all y values outside of the
interpolation range will be zero. The input workspaces require strictly
ascending x values. In case of unsorted x values, workspaces containing
point data will be sorted and a warning appears in the Results Log window.
However, only the x values will be sorted for histogram workspaces. 

If multiple spectra are defined in the WorkspaceToInterpolate workspace,
they will all be interpolated against the first spectra in
WorkspaceToMatch.

The algorithm can perform a linear interpolation, if the
WorkspaceToMatch contains two points exactly.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

Workspace characteristics (except its size and the description of the
vertical axis) can be copied from the WorkspaceToMatch (default) or
the WorkspaceToInterpolate.

Please note that the interpolation will not calculate error values.

For Histogram Workspaces
########################

If the input workspace contains histograms, rather than data points,
then SplineInterpolation will automatically convert the input to point
data. The output returned with be in the same format as the input.

Histogram workspaces being interpolated will show a warning when the
range of the data is equal to the size of the workspace to match, but
has finer bin boundaries. This is because histogram data is converted to
point data using the average of the bin boundaries. This will cause some
values to fall outside of the range of the spline when fine bin
boundaries are used.

Usage
-----

**Example - interpolate between points in one workspace to match a second workspace:**  

.. testcode:: ExSplineInterpolation

    import numpy as np

    #create a smooth function for interpolation
    dataX1 = np.arange(1, 100) * 0.07
    dataY1 = np.sin(dataX1)
    spline_ws = CreateWorkspace(dataX1, dataY1)

    #create some random points to interpolate between
    dataX2 = np.arange(1, 110, 10) * 0.07
    dataY2 = np.random.random_sample(dataX2.size) 
    ws = CreateWorkspace(dataX2, dataY2)

    #interpolate
    interpolated_ws = SplineInterpolation(WorkspaceToMatch=spline_ws, WorkspaceToInterpolate=ws)

**Example - output the derivatives of the interpolated workspace:**  

.. testcode:: ExSplineInterpolationDeriv

    import numpy as np

    #create a smooth function for interpolation
    dataX1 = np.arange(1, 100) * 0.07
    dataY1 = np.sin(dataX1)
    spline_ws = CreateWorkspace(dataX1, dataY1)

    #create some random points to interpolate between
    dataX2 = np.arange(1,110,10) * 0.07
    dataY2 = np.random.random_sample(dataX2.size) 
    ws = CreateWorkspace(dataX2, dataY2)

    #interpolate using the reference workspace and output a group workspace of derivatives for each spectrum
    interpolated_ws = SplineInterpolation(WorkspaceToMatch=spline_ws, WorkspaceToInterpolate=ws, DerivOrder=2, OutputWorkspaceDeriv='derivs')

**Example - linear interpolation when the WorkspaceToInterpolate has two points:**

.. testcode:: ExSplineInterpolationLinearOption

    import numpy as np

    #create points for interpolation
    dataX1 = np.arange(1, 50, 10) * 0.07
    dataY1 = np.sin(dataX1)
    ws_to_match = CreateWorkspace(dataX1, dataY1)

    #create two points to interpolate between
    dataX2 = np.array([0.1, 2.0])
    dataY2 = np.array([0.1, 0.15])
    ws_to_interpolate = CreateWorkspace(dataX2, dataY2)

    #interpolate linear
    interpolated_ws = SplineInterpolation(WorkspaceToMatch=ws_to_match, WorkspaceToInterpolate=ws_to_interpolate, Linear2Points=True)

**Example - change the ReferenceWorkspace:**  

.. testcode:: ExSplineInterpolation

    import numpy as np

    #create a smooth function for interpolation
    dataX1 = np.arange(1, 100) * 0.07
    dataY1 = np.sin(dataX1)
    spline_ws = CreateWorkspace(dataX1, dataY1)

    #create some random points to interpolate between
    dataX2 = np.arange(1, 110, 10) * 0.07
    dataY2 = np.random.random_sample(dataX2.size) 
    ws = CreateWorkspace(dataX2, dataY2)

    #interpolate
    interpolated_ws = SplineInterpolation(WorkspaceToMatch=spline_ws, WorkspaceToInterpolate=ws, ReferenceWorkspace='WorkspaceToInterpolate')
    
.. categories::

.. sourcelink::
