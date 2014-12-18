.. algorithm::

.. summary::

.. alias::

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

    print "Values from every 10th bin"
    print "bin\tY\tE\tY_New"
    for i in range (0,ws.blocksize(),10):
        print "%i\t%.2f\t%.2f\t%.2f" % (i,ws.readY(0)[i],ws.readE(0)[i],wsOut.readY(0)[i])


Output:

.. testoutput:: AddLogDerivative
    :options: +NORMALIZE_WHITESPACE

    Values from every 10th bin
    bin     Y       E       Y_New
    0       30.00   5.48    5.48
    10      30.00   5.48    5.48
    20      30.00   5.48    5.48
    30      30.00   5.48    5.48
    40      30.00   5.48    5.48
    50      1030.00 32.09   32.09
    60      30.00   5.48    5.48
    70      30.00   5.48    5.48
    80      30.00   5.48    5.48
    90      30.00   5.48    5.48


.. categories::
