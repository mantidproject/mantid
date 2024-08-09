.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm corrects the data and error values on a workspace by the
value of one minus an exponential function of the form
:math:`\rm C1(1 - e^{-{\rm C} x})`. This formula is calculated for each
data point, with the value of *x* being the mid-point of the bin in the
case of histogram data. The data and error values are either divided or
multiplied by the value of this function, according to the setting of
the Operation property.

This algorithm is now event aware.

This correction is applied to a copy of the input workpace and put into
output workspace. If the input and output workspaces have the same name,
the operation is applied to the workspace of that name.

Usage
-----

**Example:**

.. testcode:: ExOneMinusExp

    ws=CreateWorkspace([1,2,3],[1,1,1])
    print("You can divide the data by the factor")
    wsOut=OneMinusExponentialCor(ws,2,3,"Divide")
    print(wsOut.readY(0))

    print("Or multiply")
    wsOut=OneMinusExponentialCor(ws,2,3,"Multiply")
    print(wsOut.readY(0))


Output:

.. testoutput:: ExOneMinusExp

    You can divide the data by the factor
    [0.38550588 0.33955245 0.33416164]
    Or multiply
    [2.59399415 2.94505308 2.99256374]

.. categories::

.. sourcelink::
