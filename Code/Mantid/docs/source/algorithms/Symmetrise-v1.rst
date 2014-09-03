.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Symmetrise takes an asymmetric :math:`S(Q,w)` - i.e. one in which the
moduli of xmin & xmax are different. Typically xmax is > mod(xmin). A
negative value of x is chosen (Xcut) so that the curve for mod(Xcut) to
xmax is reflected and inserted for x less than the Xcut.

Usage
-----

**Example - Running Symmetrise on an asymmetric workspace.**

.. testcode:: ExSymmetriseSimple

    import numpy as np

    #create an asymmetric line shape
    def rayleigh(x, sigma):
      return (x / sigma**2) * np.exp( -x**2 / (2*sigma**2))

    dataX = np.arange(0, 10, 0.01)
    dataY = rayleigh(dataX, 1)

    ws = CreateWorkspace(dataX, dataY)
    ws = ScaleX(ws, -1, "Add") #centre the peak over 0

    ws = RenameWorkspace(ws, OutputWorkspace="iris00001_graphite002_red")
    Symmetrise('00001', '-0.001', InputType='Workspace')

.. categories::
