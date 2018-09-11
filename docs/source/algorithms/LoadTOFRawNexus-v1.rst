.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads a NeXus file that conforms to the TOFRaw format and
stores it in a 2D workspace. The TOFRaw format is used at SNS and
consists of a histogram representation with common bin boundaries.

Some NXS files have multiple data fields giving binning in other units
(e.g. d-spacing or momentum). You can choose which binning to use by
entering the **Signal** parameter. The default value is 1, which
normally will correspond to TOF. The "Y" units will still be in
*counts*.

The typical meanings of Signal are as follows (note that these may
change!):

-  Signal 1: Time of flight. The data field containing the bin
   boundaries is *time\_of\_flight*
-  Signal 5: q. The data field containing the bin boundaries is
   *momentum\_transfer*
-  Signal 6: d-spacing. The data field containing the bin boundaries is
   *dspacing*

.. categories::

.. sourcelink::
