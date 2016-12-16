.. _func-CrystalFieldHeatCapacity:

========================
CrystalFieldHeatCapacity
========================

.. index:: CrystalFieldHeatCapacity

Description
-----------

This function calculates the magnetic contribution to the heat capacity of a material from the splitting of its electronic energy
levels by the crystal field. It is a part of crystal field computation in Mantid and under active development. 
More documentation will follow as the development progresses.

Theory
------

The heat capacity at constant volume is given by 

.. math:: C_v = \left. \frac{\partial U}{\partial T} \right|_V = \frac{1}{k_B T^2} 
   \frac{\partial}{\partial \beta} \left[ \frac{1}{Z}\frac{\partial Z}{\partial \beta} \right] 
   \qquad \qquad \qquad \qquad \qquad \qquad \qquad \\
   = \frac{1}{k_B T^2} \left( \frac{1}{Z}\sum_n E_n^2 \exp(-\beta E_n) 
     - \left[ \frac{1}{Z}\sum_n E_n \exp(-\beta E_n) \right]^2 \right)

where :math:`k_B` is Boltzmann's constant, :math:`Z` is the partition sum, and :math:`E_n` is the n-th energy level split by the 
crystal field. This is obtained by diagonalising the crystal field Hamiltonian.

Example
-------

Here is an example of how to fit function's parameters to a spectrum. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself. For real data, you should subtract the phonon contribution manually using either
measurements from a phonon blank or a theoretical calculation (e.g. Debye model, or from lattice dynamical calculations) before
using it with this function.

The `x`-axis is given in Kelvin, and the heat capacity (`y`-axis) is in Joules per mole-Kelvin (Jmol\ :sup:`-1`\ K\ :sup:`-1`).

.. code::

    import numpy as np

    # Build a reference data set
    fun = 'name=CrystalFieldHeatCapacity,Ion=Ce,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
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
