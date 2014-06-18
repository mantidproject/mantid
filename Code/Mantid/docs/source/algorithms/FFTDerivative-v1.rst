.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calcuates the derivative of the specta in a workspace using :ref:`algm-FFT`.

Usage
-----

**Example: Different Orders of Derivative**

.. testcode:: ExFFTDerivOrders

    wsOriginal = CreateSampleWorkspace(XMax=20,BinWidth=0.2,BankPixelWidth=1,NumBanks=1)
    wsOrder1 = FFTDerivative(wsOriginal,Order=1)
    wsOrder2 = FFTDerivative(wsOriginal,Order=2)
    wsOrder3 = FFTDerivative(wsOriginal,Order=3)

    print "bin Orig  1st    2nd   3rd"
    for i in range (40,66,5):
        print "%i  %.2f  %.2f  %.2f  %.2f" % (i, wsOriginal.readY(0)[i], wsOrder1.readY(0)[i], wsOrder2.readY(0)[i], wsOrder3.readY(0)[i])

.. figure:: /images/FFTDerivativeExample.png
    :align: right
    :height: 200px

Output:

.. testoutput:: ExFFTDerivOrders

    bin Orig  1st    2nd   3rd
    40  0.47  0.69  2.47  7.26
    45  3.90  7.36  7.66  -14.40
    50  10.30  0.00  -20.41  -0.00
    55  3.90  -7.36  7.66  14.40
    60  0.47  -0.69  2.47  -7.26
    65  0.30  -0.01  0.04  -0.20


**Example: Different Orders of Derivative**

.. testcode:: ExFFTDerivOrderCheck2Ways

    wsOriginal = CreateSampleWorkspace(XMax=20,BinWidth=0.2,BankPixelWidth=1,NumBanks=1)
    wsOrder1 = FFTDerivative(wsOriginal,Order=1)
    wsOrder2 = FFTDerivative(wsOriginal,Order=2)

    wsOrder2Test = FFTDerivative(wsOrder1,Order=1)

    print "The direct 2nd order derivative and the derivative of a derivative should match"
    print CheckWorkspacesMatch(wsOrder2,wsOrder2Test,CheckAllData=True,Tolerance=1e10)

Output:

.. testoutput:: ExFFTDerivOrderCheck2Ways

    The direct 2nd order derivative and the derivative of a derivative should match
    Success!



.. categories::
