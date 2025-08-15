.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given 2 vectors defining a scattering plane and 1 peak this algorithm calculates a UB matrix that best maps the Qs calculated from hkl and Qs measured.
Found by minimising the angle of rotation about the vertical for mapping Qs calc to Qs observed.
The order in which the 2 vectors defining the plane are given to the algorithm will change the resulting UB as there are 2 equivalent UBs for the peak provided


Usage
-----

**Example:**

.. testcode:: ExFindUBFromScatteringPlane
    inst_ws = LoadEmptyInstrument(Filename="SXD_Definition.xml", OutputWorkspace="empty_SXD")
    peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=inst_ws, NumberOfPeaks=0, OutputWorkspace="peaks1")
    SetUB(peaks1, u=[1, -0.83, 0], v=[0.8, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90)
    AddPeakHKL(peaks1, [2, 2, 0])
    ClearUB(peaks1)
    FindUBFromScatteringPlane(
            vector_1=[1, -1, 0], vector_2=[1, 1, 0], a=5.4, b=5.4, c=5.4, alpha=90, beta=90, gamma=90, PeaksWorkspace=peaks1)
    print(ws.sample().getOrientedLattice().getUB())

Output:

.. testoutput::  ExFindUBFromScatteringPlane
    [[ 1.18272875e-01  1.42495894e-01 -1.48323218e-18]
    [ 0.00000000e+00  0.00000000e+00  1.85185185e-01]
    [ 1.42495894e-01 -1.18272875e-01  1.59674819e-17]]


.. categories::

.. sourcelink::
