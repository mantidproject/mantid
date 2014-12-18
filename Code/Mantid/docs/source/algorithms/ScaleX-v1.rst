.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Scales the X axis, the X-coordinate of histograms in a histogram workspace,
and the X-coordinate of events in an event workspace by the
requested amount.

-  The amount can be specified either as:
-  an absolute numerical value via the "Factor" argument or
-  an detector parameter name whose value is retrieved from the
   instrument.

Usage
-----

**Example - Modify the mean and standard deviation of a Gaussian via rescaling of the X-axis:**

.. testcode:: Ex

    import numpy as np

    # A Gaussian in the [-1, 1] range
    DataX=np.arange(-1,1,0.01)
    mean=0.3
    sigma=0.2
    DataY=np.exp( -(DataX-mean)**2/(2*sigma**2) )
    ws = CreateWorkspace(DataX,DataY)

    # Center the Gaussian by shifting the X-axis, then find its average
    ws2 = ScaleX(ws, Factor=-mean, Operation='Add')
    print 'mean=%.2f'%abs(np.sum( ws2.dataX(0) *ws2.dataY(0) ) / np.sum( ws2.dataY(0) ))

    # Decrease the standard deviation of the Gaussian by half via shrinkage of the X-axis,
    # then find its standard deviation
    ws3 = ScaleX(ws2, Factor=0.5, Operation='Multiply')
    print 'sigma=%.2f'%np.sqrt( np.sum( ws3.dataX(0)**2 *ws3.dataY(0) ) / np.sum( ws3.dataY(0) ) )

Output:

.. testoutput:: Ex

    mean=0.00
    sigma=0.10

.. categories::
