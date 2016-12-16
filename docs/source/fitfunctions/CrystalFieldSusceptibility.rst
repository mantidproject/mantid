.. _func-CrystalFieldSusceptibility:

========================
CrystalFieldSusceptibility
========================

.. index:: CrystalFieldSusceptibility

Description
-----------

This function calculates the crystal field contribution to the magnetic susceptibility using the Van Vleck formula. 
It is a part of crystal field computation in Mantid and under active development. 
More documentation will follow as the development progresses.

Theory
------

The magnetic susceptibility can be calculated by treating the magnetic field (Zeeman interaction) as a perturbation
on the crystal field energy. To second order, the susceptibility per magnetic ion is given by:

.. math:: \chi(T) = \frac{1}{Z} \sum_n \left[ \frac{| \langle V_n | g_J \mathbf{J} | V_n \rangle | ^2}{k_B T} 
   - 2 \sum_{m \neq n} \frac{| \langle V_n | g_J \mathbf{J} | V_m \rangle | ^2}{E_n - E_m} \right] \exp(-\beta E_n)

where :math:`k_B` is Boltzmann's constant, :math:`Z` is the partition sum, and :math:`V_n` and :math:`E_n` are the n-th 
wavefunction (eigenvector) and energy level (eigenvalue) of the unperturbed crystal field Hamiltonian. :math:`g_J` is the 
Landé g-factor and the moment operator is defined as 
:math:`\mathbf{J} = \hat{J}_x H_x + \hat{J}_y H_y + \hat{J}_z H_z` where :math:`\hat{J}_x`, :math:`\hat{J}_y`, and 
:math:`\hat{J}_z` are the angular momentum operators in Cartesian coordinates, with :math:`z` defined to 
be along the quantisation axis of the crystal fied (which is usually defined to be the highest symmetry rotation axis). 
:math:`H_x`, :math:`H_y`, and :math:`H_z` are the components of the unit vector pointing in the direction of the applied magnetic
field in this coordinate system.

Example
-------

Here is an example of how to fit function's parameters to a spectrum. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself. For real data, you should subtract the phonon contribution manually using either
measurements from a phonon blank or a theoretical calculation (e.g. Debye model, or from lattice dynamical calculations) before
using it with this function.

The `x`-axis is given in Kelvin, and the susceptibility (`y`-axis) is in cgs units of emu/mol.

.. code::

    import numpy as np

    # Build a reference data set
    fun = 'name=CrystalFieldSusceptibility,Ion=Ce,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
    fun += 'f0.FWHM=1.6,f1.FWHM=2.0,f2.FWHM=2.3'

    # This creates a (empty) workspace to use with EvaluateFunction
    x = np.linspace(0, 55, 200)
    y = x * 0
    e = y + 1
    ws = CreateWorkspace(x, y, e)

    # The calculated data will be in 'data', WorkspaceIndex=1
    EvaluateFunction(fun, ws, OutputWorkspace='data')
     
     # Change parameters slightly and fit to the reference data
    fun = 'name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,Temperature=44,ToleranceIntensity=0.002,B20=0.37,B22=3.9,B40=-0.03,B42=-0.1,B44=-0.12,'
    fun += 'f0.FWHM=2.2,f1.FWHM=1.8,f2.FWHM=2.1,'
    fun += 'ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0)'

    # (set MaxIterations=0 to see the starting point)
    Fit(fun, 'data', WorkspaceIndex=1, Output='fit',MaxIterations=100, CostFunction='Unweighted least squares')
    # Using Unweighted least squares fit because the data has no errors.
     
.. attributes::

.. properties::

.. categories::

.. sourcelink::
