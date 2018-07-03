.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves a reduced SANS workspace in a variety of specified formats. The supported file formats are:

- Nexus (1D/2D)
- CanSAS (1D)
- NXcanSAS (1D/2D)
- NistQxy (2D)
- RKH (1D/2D)
- CSV (1D)


Note that using the *UseZeroErrorFree* will result in outputs where the error is inflated if it had been 0. This ensures
that the particular data point with vanishing error has no significance when used during fitting.


.. categories::

.. sourcelink::
