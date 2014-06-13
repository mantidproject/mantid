.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The bin parameters are used to form an output grid. A positive
:math:`\Delta x_i\,` makes constant width bins, whilst negative ones
create logarithmic binning using the formula
:math:`x(j+1)=x(j)(1+|\Delta x_i|)\,`. The overlap of the polygons
formed from the old and new grids is tested to compute the required
signal weight for the each of the new bins on the workspace. The errors
are summed in quadrature.

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
    print ("Bins in the X axis: %i" % rb.blocksize())
    print ("Bins in the Y axis: %i" % rb.getNumberHistograms())

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
    print ("Bins in the X axis: %i" % rb.blocksize())
    print ("Bins in the Y axis: %i" % rb.getNumberHistograms())
    
Output:

.. testoutput:: ExTranspose

    Bins in the X axis: 120
    Bins in the Y axis: 200

.. categories::

