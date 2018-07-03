.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

NormaliseToUnity uses :ref:`algm-Integration` to sum up all the X
bins, then sums up the resulting spectra using
:ref:`algm-SumSpectra`. Each bin of the input workspace is then
divided by the total sum, regardless of whether a bin was included in
the sum or not. It is thus possible to normalize a workspace so that a
range of X bins and spectra sums to 1. In that case the sum of the whole
workspace will likely not be equal to 1.

Usage
-----
**Example - Normalise for 4 spectra each with 5 values**

.. testcode:: ExNormaliseToUnitySimple

   # Create a workspace with 4 spectra each with 5 values
   ws = CreateSampleWorkspace("Histogram",  NumBanks=1, BankPixelWidth=2, BinWidth=10, Xmax=50)

   # Run algorithm
   wsNorm = NormaliseToUnity (ws)

   print("Normalised Workspace")
   for i in range(4):
      print("[ {:.4f}, {:.4f}, {:.4f}, {:.4f}, {:.4f} ]".format(
            wsNorm.readY(i)[0], wsNorm.readY(i)[1], wsNorm.readY(i)[2],
            wsNorm.readY(i)[3], wsNorm.readY(i)[4]))

Output:

.. testoutput:: ExNormaliseToUnitySimple

   Normalised Workspace
   [ 0.2239, 0.0065, 0.0065, 0.0065, 0.0065 ]
   [ 0.2239, 0.0065, 0.0065, 0.0065, 0.0065 ]
   [ 0.2239, 0.0065, 0.0065, 0.0065, 0.0065 ]
   [ 0.2239, 0.0065, 0.0065, 0.0065, 0.0065 ]

.. categories::

.. sourcelink::
