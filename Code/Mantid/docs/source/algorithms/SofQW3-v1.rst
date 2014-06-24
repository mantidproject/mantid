.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts a 2D workspace from units of spectrum number, energy transfer
to the intensity as a function of momentum transfer and energy. The
rebinning is done as a weighted sum of overlapping polygons with
fractional area tracking. The result is stored in a new workspace type:
**RebinnedOutput**. The new workspace presents the data as the
fractional counts divided by the fractional area. The biggest
consequence of this method is that in places where there are no counts
and no acceptance (no fractional areas), **nan**\ s will result.

The algorithm operates in non-PSD mode by default. This means that all
azimuthal angles and widths are forced to zero. PSD mode will determine
the azimuthal angles and widths from the instrument geometry. This mode
is activated by placing the following named parameter in a Parameter
file: *detector-neighbour-offset*. The integer value of this parameter
should be the number of pixels that separates two pixels at the same
vertical position in adjacent tubes.

.. categories::
