.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::
  
Description
-----------
  
This algorithm takes the bin width at *X* of each histogram in *InputWorkspace* and calculates the average. This value is then placed in the *BinWidth* output property.

Rounding
########

If the *Rounding* property is set to **10^n**, the bin width will be rounded down to the nearest power of 10. For example, 0.11 and 0.99 will be rounded to 0.1, while 0.011 and 0.099 will be rounded to 0.01.

Restrictions on properties
################################

The *InputWorkspace* has to contain histogram data. For point data, :ref:`algm-ConvertToHistogram` can be used first, but care should be taken if the points are not equally spaced.

Usage
-----
  
**Example: rebin a workspace to equidistant bin boundaries.**

.. testcode:: ExBinWidthAtX

    import numpy
    
    # Create non-equidistant bin boundaries.
    xs = [x * x for x in  numpy.arange(0.0, 10.0, 0.05)]
    # Convert xs to numpy array.
    xs = numpy.array(xs)
    # There is one less bin than the number of boundaries.
    ys = numpy.zeros(len(xs) - 1)
    ws = CreateWorkspace(DataX=xs, DataY=ys)
    
    newWidth = BinWidthAtX(InputWorkspace=ws, X=1.0, Rounding='10^n')
    print('New bin width: {0}'.format(newWidth))
    
    # Rebin to equidistant grid
    rebinned = Rebin(InputWorkspace=ws, Params=[newWidth], FullBinsOnly=True)
    
    def firstAndLastBinWidths(workspace):
        first = workspace.readX(0)[1] - workspace.readX(0)[0]
        last = workspace.readX(0)[-1] - workspace.readX(0)[-2]
        return (first, last)
    
    first, last = firstAndLastBinWidths(ws)
    print('Bin widths before rebinning, first: {0:.4f}, last: {1:.4f}'.format(first, last))
    first, last = firstAndLastBinWidths(rebinned)
    print('Bin widths after rebinning, first: {0:.2f}, last: {1:.2f}'.format(first, last))


Output:

.. testoutput:: ExBinWidthAtX

    New bin width: 0.01
    Bin widths before rebinning, first: 0.0025, last: 0.9925
    Bin widths after rebinning, first: 0.01, last: 0.01

.. categories::

.. sourcelink::
