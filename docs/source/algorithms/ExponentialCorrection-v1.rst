.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm corrects the data and error values on a workspace by the
value of an exponential function of the form
:math:`{\rm C0} e^{-{\rm C1} x}`. This formula is calculated for each
data point, with the value of *x* being the mid-point of the bin in the
case of histogram data. The data and error values are either divided or
multiplied by the value of this function, according to the setting of
the Operation property.

Usage
-----

**Example - ExponentialCorrection with divide & multiply.**

.. testcode:: ExponentialCorrection

    ws = CreateSampleWorkspace()

    print("The first Y value before correction is: {}".format((ws.dataY(0)[1])))

    # By default, the Divide operation is used to correct the data.
    # The result is saved into another workspace, which can also be itself.
    ws_divide = ExponentialCorrection(InputWorkspace=ws,C0=2.0,C1=1.0,Operation="Divide")
    ws_multiply = ExponentialCorrection(InputWorkspace=ws,C0=2.0,C1=1.0,Operation="Multiply")

    print("The first Y value after divide correction is: {:.11e}".format(ws_divide.dataY(0)[1]))
    print("The first Y value after multiply correction is: {:.11e}".format(ws_multiply.dataY(0)[1]))

Output:

.. testoutput:: ExponentialCorrection

    The first Y value before correction is: 0.3
    The first Y value after divide correction is: 2.91363959286e+129
    The first Y value after multiply correction is: 3.08892013345e-131

.. categories::

.. sourcelink::
