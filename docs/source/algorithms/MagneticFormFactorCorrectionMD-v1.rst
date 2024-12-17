
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Scales the the signal and error of MD events by :math:`1/|F(Q)|^2` where :math:`F(Q)` is
the magnetic form factor for the ion specified in `IonName`.

`IonName` must be specified as a string with the element name followed
by a number which indicates the charge / oxidation state. E.g.
`Fe2` indicates :math:`\mathrm{Fe}^{2+}` or Fe(II).

Input workspace must be in Q with either 1 dimension of \|Q\| or 3 Q_sample/Q_lab dimensions.
It is assumed that the Q dimensions come first follow by any number of other dimensions.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - MagneticFormFactorCorrectionMD**

.. testcode:: MagneticFormFactorCorrectionMDExample

  # Create a test MD workspace
  ws = CreateMDWorkspace(Dimensions='1', Extents='1,4',
                         Names='|Q|', Units='A')
  FakeMDEventData(ws, UniformParams=-6000)

  # Run the algorithm
  wsOut = MagneticFormFactorCorrectionMD(ws, IonName="U5")

  # Bin the result so that it can be printed
  wsOut = BinMD(wsOut, AlignedDim0='|Q|,1,4,6')
  ws = BinMD(ws, AlignedDim0='|Q|,1,4,6')

  # Print the result
  print("Input signal:    ", [int(x) for x in ws.getSignalArray()])
  print("Corrected signal:", [int(x) for x in wsOut.getSignalArray()])

Output:

.. testoutput:: MagneticFormFactorCorrectionMDExample

  Input signal:     [1000, 1000, 1000, 1000, 1000, 1000]
  Corrected signal: [1137, 1281, 1498, 1813, 2270, 2937]

.. categories::

.. sourcelink::
