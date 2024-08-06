.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the weighted mean from all the spectra in a
given workspace. Monitors and masked spectra are ignored. Also,
individual bins with IEEE values will be excluded from the result. The
weighted mean calculated by the following:

:math:`\displaystyle y=\frac{\sum\frac{x_i}{\sigma^{2}_i}}{\sum\frac{1}{\sigma^{2}_i}}`

and the variance is calculated by:

:math:`\displaystyle \sigma^{2}_y=\frac{1}{\sum\frac{1}{\sigma^{2}_i}}`

Usage
-----

.. testcode:: ExWorkspace

    dataX = range(1,13)
    dataY = range(1,12)
    dataE = range(1,12)
    ws = CreateWorkspace(dataX, dataY, dataE)
    ws1 = WeightedMeanOfWorkspace(ws)
    print("Weighted Mean of Workspace: {}".format(str(ws1.readY(0))))
    print("Weighted Mean Error of Workspace: {}".format(str(ws1.readE(0))))

Output:

.. testoutput:: ExWorkspace

    Weighted Mean of Workspace: [1.93826376]
    Weighted Mean Error of Workspace: [1.2482116]

.. categories::

.. sourcelink::
