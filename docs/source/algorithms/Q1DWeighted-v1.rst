.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs azimuthal averaging for a 2D SANS data set by going through
each detector pixel, determining its Q-value, and adding its amplitude
:math:`I` to the appropriate Q bin. For greater precision, each detector
pixel can be sub-divided in sub-pixels by setting the ``NPixelDivision``
parameters. Each pixel has a weight of 1 by default, but the weight of
each pixel can be set to :math:`1/\Delta I^2` by setting the
``ErrorWeighting`` parameter to True.

See the :ref:`Rebin <algm-Rebin>` documentation for details about choosing the ``OutputBinning`` parameter.

See `SANSReduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`__
documentation for calculation details.


Usage
-----

This algorithm is not intended to be run individually, rather as a part of the `SANSReduction <http://www.mantidproject.org/Reduction_for_HFIR_SANS>`_.

.. categories::

.. sourcelink::
