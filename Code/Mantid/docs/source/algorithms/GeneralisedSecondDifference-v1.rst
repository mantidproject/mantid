.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compute the generalised second difference of a spectrum or several
spectra based on the method described by M.A. Mariscotti., Nuclear
Instruments and Methods 50, 309 (1967). Given a spectrum with value yi
(0<=i<n), the generalised second difference corresponds to the second
difference curve, smoothed by averaging each point in the interval
[-m,+m], and applying this procedure z times.

Usage
-----

**Example - Use on a single peak**

.. testcode:: ExGeneralisedSecondDifferenceSimple

   # Create Workspace with peak around x=15.0
   ws = CreateSampleWorkspace(BankPixelWidth=1, Xmax=30, BinWidth=1)

   # Apply algorithm. 
   wsD = GeneralisedSecondDifference(ws,M=2,Z=2)

   # Show values around the peak
   print "%.2f, %.2f, %.2f, %.2f, %.2f" % (wsD.readY(0)[8], wsD.readY(0)[9], wsD.readY(0)[10], wsD.readY(0)[11], wsD.readY(0)[12])
   print "Peak at", wsD.readX(0)[10]

Output:

.. testoutput:: ExGeneralisedSecondDifferenceSimple

   -0.34, -7.21, -20.00, -7.21, -0.34
   Peak at 15.0

.. categories::
