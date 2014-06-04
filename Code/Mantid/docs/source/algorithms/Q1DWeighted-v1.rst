.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs azimuthal averaging for a 2D SANS data set by going through
each detector pixel, determining its Q-value, and adding its amplitude
:math:`I` to the appropriate Q bin. For greater precision, each detector
pixel can be sub-divided in sub-pixels by setting the *NPixelDivision*
parameters. Each pixel has a weight of 1 by default, but the weight of
each pixel can be set to :math:`1/\Delta I^2` by setting the
*ErrorWeighting* parameter to True.

See the `Rebin <http://www.mantidproject.org/Rebin>`__ documentation for
details about choosing the *OutputBinning* parameter.

See `SANS
Reduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`__
documentation for calculation details.

.. categories::
