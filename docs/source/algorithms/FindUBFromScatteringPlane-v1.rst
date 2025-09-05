.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given 2 vectors defining a scattering plane, 1 peak and lattice parameters this algorithm calculates a UB matrix that best maps the Qs calculated from hkl and Qs measured.
Angle is found by minimising the norm(qsample_predicted-qsample_observed).
The order of the two input vectors dictates the sign of the orthogonal direction produced by the cross-product - however the resulting UB matrices are equivalent

Usage
-----

**Example:**

.. testcode:: ExFindUBFromScatteringPlane

    inst_ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
    peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=inst_ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
    SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
    AddPeakHKL(peaks1, [2, 2, 0])
    ClearUB(peaks1)
    FindUBFromScatteringPlane(Vector1=[1, -1, 0], Vector2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace=peaks1)
    print(np.round(peaks1.sample().getOrientedLattice().getUB(),4))

Output:

.. testoutput::  ExFindUBFromScatteringPlane

    [[ 0.1183  0.1425 -0.    ]
     [ 0.      0.      0.1852]
     [ 0.1425 -0.1183  0.    ]]



.. categories::

.. sourcelink::
