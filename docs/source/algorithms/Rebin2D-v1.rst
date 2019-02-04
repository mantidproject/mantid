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

If the input workspace is not of type: **RebinnedOutput**, and the
`UseFractionalArea` option is set to `True`, the algorithm
will assume that the input fraction is unity. This is correct if this
workspace has not been previously rebinned, but will give incorrect
error (standard deviation) estimates if it has been rebinned.

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

.. categories::

.. sourcelink::

