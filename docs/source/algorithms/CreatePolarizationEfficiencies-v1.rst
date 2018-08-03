
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a workspace in which the spectra contain the polarization efficiencies calculated from polynomial coefficients
on the x-values of the input workspace.


Usage
-----

**Example**

.. testcode:: Example

    ws = CreateWorkspace([0, 1, 2, 3, 4], [0, 0, 0, 0, 0])
    eff = CreatePolarizationEfficiencies(ws, Pp=[0, 1, 2, 3], Ap=[1, 2, 3], Rho=[3, 2, 1], Alpha=[4, 3, 2, 1])
    print(eff.getAxis(1).label(0))
    print(eff.getAxis(1).label(1))
    print(eff.getAxis(1).label(2))
    print(eff.getAxis(1).label(3))


Output:

.. testoutput:: Example

    Pp
    Ap
    Rho
    Alpha

.. categories::

.. sourcelink::
