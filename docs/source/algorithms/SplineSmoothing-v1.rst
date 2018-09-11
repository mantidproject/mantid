.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm performs a smoothing of the input data using a cubic
spline. The algorithm takes a 2D workspace and generates a spline 
for each of the spectra to approximate a fit of the data.

Optionally, this algorithm can also calculate the first and second
derivatives of each of the interpolated points as a side product.
Setting the DerivOrder property to zero will force the algorithm to
calculate no derivatives.

The algorithm provides user with an option of including number of
breaking points which will enable the algorithm to execute functions
with large number of spikes or noise. With the control of number of
breaking points, user will be able to execute
:ref:`SplineSmoothing <algm-SplineSmoothing-v1>` algorithm in small
period of time. The lower breaking points number is set the faster
algorithm will execute the function but it will not produce high
quality smoothing function. By inserting high number of breaking points,
a detailed smoothing function can also be produced. By not providing
MaxNumberOfBreaks, the alogirthm will assume that user would like to
execute the maximum number of breaks possible. This comes with a risk
of alogirthm falling in to a very long loop.

By providing MaxNumberOfBreaks as a parameter, the users are also will
be able to successfully and efficiently apply the algorithm to a workspace
with multiple spectrums, which would generate an output of workspace
with multiple spectrums of :ref:`SplineSmoothing <algm-SplineSmoothing-v1>`
algorithm applied to it. `BSpline <http://www.mantidproject.org/BSpline>`_
can be used to help you understand break-points in further detail. 


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

    print("This has created a spline for each spectra in the %s workspace" % wsOut)

    try:
        plotSpectrum([ws,wsOut],0)
    except NameError:
        #plotSpectrum was not available, Mantidplot is probably not running
        pass


Output:

.. testoutput:: SmoothNoisy

    This has created a spline for each spectra in the wsOut workspace

.. categories::

.. sourcelink::
