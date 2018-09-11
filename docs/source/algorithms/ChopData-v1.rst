.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will chop the input workspace into equally sized
workspaces, and adjust the X-values given so that they all begin from
the same point. This is useful if your raw files contain multiple
frames.

Identifying Extended Frames
###########################

.. figure:: /images/ChopDataIntegrationExplanation.png
   :alt: Figure 1: Example Monitor Spectrum with Extended Frames

   Figure 1: Example Monitor Spectrum with Extended Frames

If the parameters *IntegrationRangeLower*, *IntegrationRangeUpper* and
*MonitorWorkspaceIndex* are provided to the algorithm, then it will
attempt to identify where in the workspace the frames have been
extended.

For example: looking at Figure 1 which shows an input workspace covering
100000 microseconds, we can see that the first frame covers forty
thousand, and the other three cover twenty thousand each.

In order for Mantid to determine this programatically, it integrates
over a range (defined by IntegrationRangeLower and
IntegrationRangeUpper) for each "chop" of the data. If the relative
values for this integration fall within certain bounds, then the chop is
deemed to be a continuation of the previous one rather than a separate
frame. If this happens, then they will be placed in the same workspace
within the result group.

The algorithm will only look at the workspace given in
*MonitorWorkspaceIndex* property to determine this. Though it is
expected and recommended that you use a monitor spectrum for this
purpose, it is not enforced so you may use a regular detector if you
have cause to do so.

Usage
-----

**Example - Roughly Chop Workspace**

.. include:: ../usagedata-note.txt

.. testcode:: Ex

   # Creates a dummy ws with a single spectrum with 100 bins.  There is exactly 1 count in every bin.
   ws = CreateSampleWorkspace("Histogram", "Flat background", NumBanks=1, BankPixelWidth=1)

   time_diff = (ws.readX(0)[99] - ws.readX(0)[0])

   # Chop the workspace roughly in two.
   result = ChopData(ws, NChops=2, Step=time_diff/2)

   print("The time range of the original workspace was {:.0f}.".format(time_diff))
   print("The number of bins in the orginal workspace was {}.".format(ws.blocksize()))
   print("The number of bins in the 1st chop is {}.".format(result[0][0].blocksize()))
   print("The number of bins in the 2nd chop is {}.".format(result[0][1].blocksize()))

Output:

.. testoutput:: Ex

   The time range of the original workspace was 19800.
   The number of bins in the orginal workspace was 100.
   The number of bins in the 1st chop is 48.
   The number of bins in the 2nd chop is 48.

.. categories::

.. sourcelink::
