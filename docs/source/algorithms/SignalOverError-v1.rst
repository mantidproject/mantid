.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Take a :ref:`MatrixWorkspace <MatrixWorkspace>` as input, and replaces the
Y values by Y/E (signal divided by error)


Usage
-----

**Example: Taking the derivative of logs**

.. testcode:: AddLogDerivative
    
    ws = CreateSampleWorkspace("Event")
    wsOut = SignalOverError(ws)

    print("Values from every 10th bin")
    print("bin\tY\tE\tY_New")
    for i in range (0,ws.blocksize(),10):
        print("{}\t{:.2f}\t{:.2f}\t{:.2f}".format(i,ws.readY(0)[i],ws.readE(0)[i],wsOut.readY(0)[i]))


Output:

.. testoutput:: AddLogDerivative
    :options: +NORMALIZE_WHITESPACE

    Values from every 10th bin
    bin     Y       E       Y_New
    0       7.00   2.65    2.65
    10      7.00   2.65    2.65
    20      7.00   2.65    2.65
    30      7.00   2.65    2.65
    40      7.00   2.65    2.65
    50      257.00 16.03   16.03
    60      7.00   2.65    2.65
    70      7.00   2.65    2.65
    80      7.00   2.65    2.65
    90      7.00   2.65    2.65


.. categories::

.. sourcelink::
