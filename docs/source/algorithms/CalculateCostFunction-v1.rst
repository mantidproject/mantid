
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates a value of any available cost function. Returns the calculated value in the `Value` property. The input properties have the same meaning
and behaviour as in :ref:`algm-Fit`.

Usage
-----
**Example**

.. testcode:: CalculateCostFunctionExample

    import numpy as np

    # Create a data set
    x = np.linspace(0,1,10)
    y = 1.0 + 2.0 * x
    e = np.sqrt(y)
    ws = CreateWorkspace(DataX=x, DataY=y, DataE=e)

    # Define a function
    func = 'name=LinearBackground,A0=1.1,A1=1.9'

    # Calculate the chi squared by default
    value = CalculateCostFunction(func, ws)
    print('Value of least squares is {:.13f}'.format(value))

    # Calculate the unweighted least squares
    value = CalculateCostFunction(func, ws, CostFunction='Unweighted least squares')
    print('Value of unweighted least squares is {:.13f}'.format(value))


Output:

.. testoutput:: CalculateCostFunctionExample

    Value of least squares is 0.0133014391988
    Value of unweighted least squares is 0.0175925925926


.. categories::

.. sourcelink::
