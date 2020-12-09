.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm provides a way to convert from a `MDEventWorkspace` in QSample units to a `MDHistoWorkspace` in HKL
units. By default, the UB matrix from the `InputWorkspace` is used unless `FindUBFromPeaks` is true. In this case, the
UB matrix from the specified `PeaksWorkspace` will be used. An optional transformation can be applied to the peak
UB matrix to better align peaks in HKL space (see :ref:`TransformHKL <algm-TransformHKL>`).

.. categories::

.. sourcelink::
