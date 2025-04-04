.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The bin parameters are used to form an output grid. A positive
:math:`\Delta x_i\,` makes constant width bins, whilst negative ones
create logarithmic binning using the formula
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`. The overlap of the polygons
formed from the old and new grids is tested to compute the required
signal weight for the each of the new bins on the workspace, like in
:ref:`algm-Rebin`, and the errors are summed in quadrature, as:

.. math:: Y^{\mathrm{new}} = \sum_i Y^{\mathrm{old}}_i F_i
.. math:: E^{\mathrm{new}} = \sqrt{\sum_i (E^{\mathrm{old}}_i)^2 F_i}

where :math:`F_i = A^{\mathrm{overlap}}_i / A^{\mathrm{old}}_i` is the
ratio of the overlap area of the new and old bin over the area of the
old bin.

If the input to :ref:`algm-Rebin2D` is a **RebinnedOutput** workspace,
the algorithm will make use of the fractional weights stored in each bin of
the input **RebinnedOutput** workspace. A more detailed explanation of this
approach is described in :ref:`FractionalRebinning <FractionalRebinning>`.

If the `UseFractionalArea` option is set to `True` and the input workspace
is not of type **RebinnedOutput**, the algorithm will be correct if the input
workspace has not been previously rebinned, but will give incorrect
error (standard deviation) estimates if it has been rebinned. You can find more
information on why this is the case in :ref:`FractionalRebinning <FractionalRebinning>`.

.. note:: Rebin2D looks at the histogram representation of the input
          workspace, so while it will accept EventWorkspaces, number of bins
          will affect the numerical results.

Requirements
------------

The algorithms currently requires the second axis on the workspace to be
a numerical axis so :ref:`algm-ConvertSpectrumAxis` may
need to run first.

Usage
-----

**Example - A simple example using the fractional area**

.. testcode:: ExUseFractionalArea

    ws = CreateSampleWorkspace()
    #Convert the Spectrum Axis to theta
    wsc = ConvertSpectrumAxis(ws,"theta")

    rb = Rebin2D(wsc,[0,100,20000],[0,0.01,1.2],UseFractionalArea=True)
    print("Bins in the X axis: {}".format(rb.blocksize()))
    print("Bins in the Y axis: {}".format(rb.getNumberHistograms()))

Output:

.. testoutput:: ExUseFractionalArea

    Bins in the X axis: 200
    Bins in the Y axis: 120

**Example - Transposing the Result**

.. testcode:: ExTranspose

    ws = CreateSampleWorkspace()
    #Convert the Spectrum Axis to theta
    wsc = ConvertSpectrumAxis(ws,"theta")

    rb = Rebin2D(wsc,[0,100,20000],[0,0.01,1.2],Transpose=True)
    print("Bins in the X axis: {}".format(rb.blocksize()))
    print("Bins in the Y axis: {}".format(rb.getNumberHistograms()))

Output:

.. testoutput:: ExTranspose

    Bins in the X axis: 120
    Bins in the Y axis: 200

**Example - Rebinning twice preserves signal/errors**

.. testcode:: ExRebinTwice

    import numpy as np
    # prepare an input workspace
    theta_tof = CreateSampleWorkspace()
    theta_tof = ConvertSpectrumAxis(theta_tof, "theta")

    theta_tof_rb1 = Rebin2D(theta_tof, '100,400,20000', '0, 0.001,1', UseFractionalArea=True)
    theta_tof_rb2 = Rebin2D(theta_tof_rb1, '100,400,20000', '0, 0.004,1', UseFractionalArea=True)
    theta_tof_rb_final = Rebin2D(theta_tof,  '100,400,20000', '0, 0.004, 1', UseFractionalArea=True)
    print(f'Signal difference = {np.median(np.abs(theta_tof_rb_final.readY(0) - theta_tof_rb2.readY(0))):.3f}')
    print(f'Errors difference = {np.median(np.abs(theta_tof_rb_final.readE(0) - theta_tof_rb2.readE(0))):.3f}')

.. testoutput:: ExRebinTwice

    Signal difference = 0.000
    Errors difference = 0.000


.. categories::

.. sourcelink::
