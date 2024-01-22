.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Function to symmeterise an :ref:`MDHistoWorkspace <MDHistoWorkspace>` with 3 dimensions (HKL) using the
symmetry operations of the point group of the Laue class (inferred from the ``PointGroup`` or ``Spacegroup`` provided).

Two types of average can be performed:

    1. ``WeightedAverage=False`` (simple mean of the signal values and errors are added in quadrature then divided by the number of contributing bins)
    2. ``WeightedAverage=True`` (weighted average performed using inverse square of errors as weights).

If (2) ``WeightedAverage=True`` then the signal is given by :math:`s = \Sigma_i w_i s_i` where
:math:`s_i` is the signal in symmetrically equivalent bin :math:`i` and the weights are the inverse of the
error squared in each bin, :math:`w_i = 1/e_i^2`. The final error is given by :math:`1/e^2 = \Sigma_i w_i`.


Usage
-----

**Example:**

.. testcode:: SymmetriseMDHisto

    from mantid.simpleapi import *

    y = np.r_[4*[1], 4*[0]]
    nbins = int(len(y) ** (1 / 3))
    ws = CreateMDHistoWorkspace(SignalInput=y, ErrorInput=y,
                                     Dimensionality=3, Extents='-1,1,-1,1,-1,1',
                                     NumberOfBins=3 * [nbins], Names='H,K,L',
                                     Units='rlu,rlu,rlu')

    ws_sym = SymmetriseMDHisto(ws, PointGroup="-1", WeightedAverage=False)

    print("All HKL voxels have non-zero intensity?", np.all(ws_sym.getSignalArray()))

Output:

.. testoutput:: SymmetriseMDHisto

    All HKL voxels have non-zero intensity? True

.. categories::

.. sourcelink::