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

    # create an asymmetric line shape
    def rayleigh(x, sigma):
      return (x / sigma ** 2) * np.exp(-x ** 2 / (2 * sigma ** 2))

    data_x = np.arange(0, 10, 0.01)
    data_y = rayleigh(data_x, 1)

    sample_ws = CreateWorkspace(data_x, data_y)
    sample_ws = ScaleX(sample_ws, -1, "Add")  # centre the peak over 0

    symm_ws = Symmetrise(Sample=sample_ws, XCut=0.05)

.. categories::
