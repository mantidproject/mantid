.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm to mask detectors in particular banks, tube, or pixels. It
applies to the following instruments only: ARCS, CNCS, HYSPEC, NOMAD,
POWGEN, SEQUOIA, SNAP, TOPAZ. For instruments with rectangular position
sensitive detectors (POWGEN, SNAP, TOPAZ), the tube is corresponding to
the x coordinate, and pixel to the y coordinate. For example, on SNAP
Bank="1", Tube="3" corresponds to 'SNAP/East/Column1/bank1/bank1(x=3)',
and Bank="1", Tube="3", Pixel="5" is
'SNAP/East/Column1/bank1/bank1(x=3)/bank1(3,5)'.

If one of Bank, Tube, Pixel entries is left blank, it will apply to all
elements of that type. For example:

MaskBTP(w,Bank = "1") will completely mask all tubes and pixels in bank
1. MaskBTP(w,Pixel = "1,2") will mask all pixels 1 and 2, in all tubes,
in all banks.

The algorithm allows ranged inputs: Pixel = "1-8,121-128" is equivalent
to Pixel = "1,2,3,4,5,6,7,8,121,122,123,124,125,126,127,128"

'''Note: '''Either the input workspace or the instrument must be set. If
the workspace is set, the instrument is ignored.

.. categories::
