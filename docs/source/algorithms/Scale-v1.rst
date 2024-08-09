.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses the binary operation algorithms :ref:`algm-Multiply` or
:ref:`algm-Plus` to scale the input workspace by the amount requested.
This algorithm is provided as a simple, but less powerful, alternative
to the python :ref:`workspace algebra <MatrixWorkspace Algebra>` functionality.


Usage
-----

**Example: Adding an offset**

.. testcode:: ExOffsetScale

    ws = CreateSampleWorkspace(BankPixelWidth=1)
    print("Every 10th bin value of " + ws.name())
    print(ws.readY(0)[0:100:10])

    # Add 2 using scale
    wsOffset = Scale(ws,2,"Add")
    print("Every 10th bin value of " + wsOffset.name())
    print(wsOffset.readY(0)[0:100:10])

    # Add 2 using the workspace operator overloads
    wsOffset2 = ws + 2
    print("Every 10th bin value of " + wsOffset2.name())
    print(wsOffset2.readY(0)[0:100:10])

Output:

.. testoutput:: ExOffsetScale

    Every 10th bin value of ws
    [ 0.3  0.3  0.3  0.3  0.3 10.3  0.3  0.3  0.3  0.3]
    Every 10th bin value of wsOffset
    [ 2.3  2.3  2.3  2.3  2.3 12.3  2.3  2.3  2.3  2.3]
    Every 10th bin value of wsOffset2
    [ 2.3  2.3  2.3  2.3  2.3 12.3  2.3  2.3  2.3  2.3]

**Example: Multiplying by a value**

.. testcode:: ExOffsetScale

    ws = CreateSampleWorkspace(BankPixelWidth=1)
    print("Every 10th bin value of " + ws.name())
    print(ws.readY(0)[0:100:10])

    # Multiply by 10 using scale
    wsScaled = Scale(ws,10,"Multiply")
    print("Every 10th bin value of " + wsScaled.name())
    print(wsScaled.readY(0)[0:100:10])

    # Multiply by 10 using the workspace operator overloads
    wsScaled2 = ws * 10
    print("Every 10th bin value of " + wsScaled2.name())
    print(wsScaled2.readY(0)[0:100:10])

Output:

.. testoutput:: ExOffsetScale

    Every 10th bin value of ws
    [ 0.3  0.3  0.3  0.3  0.3 10.3  0.3  0.3  0.3  0.3]
    Every 10th bin value of wsScaled
    [  3.   3.   3.   3.   3. 103.   3.   3.   3.   3.]
    Every 10th bin value of wsScaled2
    [  3.   3.   3.   3.   3. 103.   3.   3.   3.   3.]



.. categories::

.. sourcelink::
