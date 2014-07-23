.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a `PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_ and a set of
lattice parameters, attempts to tag each peak with a HKL value by comparing
d-spacings between potential HKL matches and the peaks as well as angles between Q
vectors.

Usage Notes
-----------

This algorithm does not generate a UB Matrix, it will only index peaks.
Run :ref:`CalculateUMatrix <algm-CalculateUMatrix>`
algorithm after executing this algorithm in order
to attach a UB Matrix onto the sample. The :ref:`CopySample <algm-CopySample>`
algorithm will allow this UB Matrix to be transfered between workspaces.

Usage
-----

.. warning::

    This algorithm is deprecated and should no longer be used. One should consider using
    :ref:`FindPeaksMD <algm-FindPeaksMD>`

.. categories::
