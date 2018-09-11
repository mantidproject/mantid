.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is the equivalent of the COLETTE "DISPLAY H/V" command.
It firsts integrates the input workspace, which must contain all the
spectra from the detector of interest - no more and no less (so 128x128
or 192x192), between the X values given. Then each row or column is
summed between the H/V\_Min/Max values, if given, and the result is a
single spectrum of row or column number against total counts.

ChildAlgorithms used
####################

The :ref:`algm-Integration` algorithm is used to sum up each
spectrum between XMin & XMax.

Usage
-----
**Example - Sum rows of a 128*128 workspace**

.. testcode:: ExSumRowColumnSimple

   # Create a workspace with 128*128 spectra each with 5 values (only 128*128 or 192*192 are valid)
   ws = CreateSampleWorkspace("Histogram",  NumBanks=1, BankPixelWidth=128, BinWidth=10, Xmax=50)

   # Run algorithm with Horizontal orientation
   OutputWorkspace = SumRowColumn( ws, "D_H")

   print("Input workspace has {} points.".format(ws.getNPoints()))
   print("Output workspace has {} points.".format(OutputWorkspace.getNPoints()))
   
Output:

.. testoutput:: ExSumRowColumnSimple

   Input workspace has 81920 points.
   Output workspace has 128 points.

.. categories::

.. sourcelink::
