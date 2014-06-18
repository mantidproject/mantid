.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs interpolation of points onto a cubic spline. The
algorithm takes two input workspaces: one that is used to define the
spline and one that contains a number of spectra to be interpolated onto
the spline.

If multiple spectra are defined in the WorkspaceToInterpolate workspace,
they will all be interpolated against the first spectra in
WorkspaceToMatch.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

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

**Example - interpolate between points in one workspace to match a reference workspace:**  

.. testcode:: ExSplineInterpolationSimple

    import numpy as np

    #create a smooth function for interpolation
    dataX1 = np.arange(1, 100) * 0.07
    dataY1 = np.sin(dataX1)
    spline_ws = CreateWorkspace(dataX1, dataY1)

    #create some random points to interpolate between
    dataX2 = np.arange(1,110,10) * 0.07
    dataY2 = np.random.random_sample(dataX2.size) 
    ws = CreateWorkspace(dataX2, dataY2)

    #interpolate using the reference workspace
    interpolated_ws = SplineInterpolation(WorkspaceToMatch=spline_ws, WorkspaceToInterpolate=ws, DerivOrder=0)
    
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

.. categories::
