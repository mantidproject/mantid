.. algorithm::

.. summary::

.. alias::

.. properties::
  
Description
-----------
  
This algorithm calculates the median bin widht of each histogram in *InputWorkspace*. The (optionally rounded) mean value of the medians is then placed in the *BinWidth* output property.

Workspaces with Point data are treated as in :ref:`algm-Rebin`: they are converted to histograms with :ref:`algm-ConvertToHistogram` before the bin widths are checked and afterwards back using :ref:`algm-ConvertToPointData`.

Rounding
########

If the *Rounding* property is set to **10^n**, the bin width will be rounded down to the nearest power of 10. For example, 0.11 and 0.99 will be rounded to 0.1, while 0.011 and 0.099 will be rounded to 0.01.


Usage
-----
  
**Example: rebinning a workspace.**

.. testCode:: Example

    import numpy
    
    # For normal python lists: 3 * [0.1] = [0.1, 0.1, 0.1]
    binWidths = 3 * [0.1] + 4 * [1.7] + [10.0]
    # Convert to numpy array.
    # For numpy arrays: 3 * [0.1] = [0.3], thus the above magic would not work.
    binWidhts = numpy.array(binWidths)
    # Make bin boundaries out of the widths. The first boundary is at -3.0.
    xs = numpy.cumsum(numpy.append(numpy.array([-3.0]), binWidths))
    # There is one less bin than the number of boundaries.
    ys = numpy.zeros(len(xs) - 1)
    ws = CreateWorkspace(DataX=xs, DataY=ys)
    
    newWidth = MedianBinWidth(InputWorkspace=ws)
    print('New bin width: {0}'.format(newWidth))
    
    rebinned = Rebin(InputWorkspace=ws, Params=[newWidth], FullBinsOnly=True)
    
    widths = ws.readX(0)[1:] - ws.readX(0)[:-1]
    print('Bin widths before rebinning: {0}'.format(widths))
    widths = rebinned.readX(0)[1:] - rebinned.readX(0)[:-1]
    print('Bin widths after rebinning: {0}'.format(widths))

Output:

.. testOutput:: Example

    New bin width: 1.7
    Bin widths before rebinning: [  0.1   0.1   0.1   1.7   1.7   1.7   1.7  10. ]
    Bin widths after rebinning: [ 1.7  1.7  1.7  1.7  1.7  1.7  1.7  1.7  1.7  1.7]

.. categories::

.. sourcelink::


