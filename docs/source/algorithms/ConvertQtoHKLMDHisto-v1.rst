.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm provides a way to convert from a `MDEventWorkspace` in QSample units to a `MDHistoWorkspace` in HKL
units. By default, the :ref:`UB matrix <Lattice>` from the `InputWorkspace` is used unless `PeaksWorkspace` is provided. In this case,
the :ref:`UB matrix <Lattice>` from the specified `PeaksWorkspace` will be used instead.

Note that this algorithm does not take into account normalization of the output, it only bins the data in HKL. See
:ref:`MDNorm <algm-MDNorm>` (if using TOF) to account for normalizing when converting to HKL.

.. categories::

.. sourcelink::
