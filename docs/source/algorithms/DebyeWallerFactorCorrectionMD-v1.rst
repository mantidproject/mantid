
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Scales the the signal and error of MD events by the inverse of Debye-Waller Factor :math:`exp(<q>^2 \times <u^2>)` where :math:`<u^2>` is
the `Mean squared displacement`.

`Mean squared displacement` must be a positive float number.

Input workspace must be in Q with either 1 dimension of \|Q\| or 3 Q_sample/Q_lab dimensions.
It is assumed that the Q dimensions come first follow by any number of other dimensions.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - DebyeWallerFactorCorrectionMD**

.. testcode:: DebyeWallerFactorCorrectionMDExample

  # Create a test MD workspace
  ws = CreateMDWorkspace(Dimensions='1', Extents='1,4',
                         Names='|Q|', Units='A')
  FakeMDEventData(ws, UniformParams=-6000)

  # Run the algorithm
  wsOut = DebyeWallerFactorCorrectionMD(ws, 0.15)

  # Bin the result so that it can be printed
  wsOut = BinMD(wsOut, AlignedDim0='|Q|,1,4,6')
  ws = BinMD(ws, AlignedDim0='|Q|,1,4,6')

  # Print the result
  print("Input signal:    ", [int(x) for x in ws.getSignalArray()])
  print("Corrected signal:", [int(x) for x in wsOut.getSignalArray()])

Output:

.. testoutput:: MagneticFormFactorCorrectionMDExample

  Input signal:     [1000, 1000, 1000, 1000, 1000, 1000]
  Corrected signal: [1082, 1167, 1289, 1462, 1699, 2024]

.. categories::

.. sourcelink::
