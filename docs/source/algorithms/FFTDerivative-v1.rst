.. algorithm::

.. summary::

.. relatedalgorithms::

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

    print("bin Orig  1st    2nd   3rd")
    for i in range (41,67,5):
        print("{}  {:.2f}  {:.2f}  {:.2f}  {:.2f}".format(i, wsOriginal.readY(0)[i], wsOrder1.readY(0)[i], wsOrder2.readY(0)[i], wsOrder3.readY(0)[i]))

.. figure:: /images/FFTDerivativeExample.png
    :align: right
    :height: 200px

Output:

.. testoutput:: ExFFTDerivOrders

    bin Orig  1st    2nd   3rd
    41  0.67  1.35  4.20  9.93
    46  5.50  8.50  3.25  -29.37
    51  9.90  -3.92  -17.99  23.34
    56  2.60  -5.63  9.10  0.70
    61  0.37  -0.32  1.30  -4.51
    66  0.30  -0.00  0.01  -0.07


**Example: Different Orders of Derivative**

.. testcode:: ExFFTDerivOrderCheck2Ways

    wsOriginal = CreateSampleWorkspace(XMax=20,BinWidth=0.2,BankPixelWidth=1,NumBanks=1)
    wsOrder1 = FFTDerivative(wsOriginal,Order=1)
    wsOrder2 = FFTDerivative(wsOriginal,Order=2)

    wsOrder2Test = FFTDerivative(wsOrder1,Order=1)

    print("The direct 2nd order derivative and the derivative of a derivative should match")
    print(CompareWorkspaces(wsOrder2,wsOrder2Test,CheckAllData=True,Tolerance=1e10)[0])

Output:

.. testoutput:: ExFFTDerivOrderCheck2Ways

    The direct 2nd order derivative and the derivative of a derivative should match
    True

.. categories::

.. sourcelink::
