.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Finds peaks for the given `MDWorkspace` in Q-space and sets the UB matrix, refining it for a specific `CellType`
and `Centering`. If the lattice parameters are specified, then the UB matrix will be found with those parameters by
:ref:`FindUBUsingLatticeParameters <algm-FindUBUsingLatticeParameters>`. A `PeaksWorkspace` is created containing the
new UB matrix with peaks indexed and found in Q-space.

The input to this algorithm is intended to be DEMAND data that has been processed from
:ref:`HB3AAdjustSampleNorm <algm-HB3AAdjustSampleNorm>`.

.. categories::

.. sourcelink::
