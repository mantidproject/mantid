.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs a smoothing of the input data using a cubic
spline. The algorithm takes a 2D workspace and generates a spline for
each of the spectra to approximate a fit of the data.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

For Histogram Workspaces
########################

If the input workspace contains histograms, rather than data points,
then SplineInterpolation will automatically convert the input to point
data. The output returned with be in the same format as the input.

Usage
-----

**Example - Smoothing noisy data:**

.. testcode:: SmoothNoisy

    ws = CreateSampleWorkspace("Histogram","Multiple Peaks", 
        BankPixelWidth=1, NumBanks=10, Random=True,
        XMax=30, BinWidth=0.3)
    wsOut = SplineSmoothing(ws,Error=1)

    print "This has created a spline for each spectra in the %s workspace" % wsOut

    try:
        plotSpectrum([ws,wsOut],0)
    except NameError:
        #plotSpectrum was not available, Mantidplot is probably not running
        pass


Output:

.. testoutput:: SmoothNoisy

    This has created a spline for each spectra in the wsOut workspace

.. categories::
