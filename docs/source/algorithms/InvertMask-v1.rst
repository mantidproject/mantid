.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A NOT operation will be conducted on the input masking workspace
(SpecialWorkspace2D)

Output
######

A SpecialWorkspace2D with the same dimension and geometry as the input
two SpecialWorkspace2D.


Usage
-----

**Example - invert a POWGEN mask:**

.. testcode:: ExInvertPG3

  # Load data
  maskws = LoadMask(Instrument="POWGEN", InputFile="Mask-PG3-19884.xml")

  # Check source mask workspace
  nummasked = 0
  for i in xrange(maskws.getNumberHistograms()):
    if maskws.readY(i)[0] > 0.5:
      nummasked += 1

  # Invert mask
  invmaskws = InvertMask(InputWorkspace=maskws)

  # Check target mask workspace
  nummasked2 = 0
  for i in xrange(invmaskws.getNumberHistograms()):
    if invmaskws.readY(i)[0] > 0.5:
      nummasked2 += 1

  # Print out
  print "Number of histogram: ", maskws.getNumberHistograms()
  print "Source Mask workspace # Detectors masked = ", nummasked
  print "Source Mask workspace # Detectors masked = ", nummasked2

Output:

.. testoutput:: ExInvertPG3

  Number of histogram:  33418
  Source Mask workspace # Detectors masked =  82
  Source Mask workspace # Detectors masked =  33336

.. categories::

.. sourcelink::
