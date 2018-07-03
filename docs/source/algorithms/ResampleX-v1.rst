.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This method will resample the x-axis with the number of specified bins.
This is done by setting up appropriate parameters to call
:ref:`Rebin <algm-Rebin>` with. If the ``XMin`` and ``XMax`` parameters are supplied
it will use those as the range, they can be supplied as a comma delimited
list or as a single value.

The ``LogBinning`` option calculates constant delta-X/X binning and rebins
using that, otherwise the bins are constant width.

.. categories::

.. sourcelink::
