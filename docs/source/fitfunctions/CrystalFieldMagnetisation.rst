.. _func-CrystalFieldMagnetisation:

========================
CrystalFieldMagnetisation
========================

.. index:: CrystalFieldMagnetisation

Description
-----------

This function calculates the crystal field contribution to the magnetisation as a function of applied magnetic
in a specified direction, in either atomic (:math:`\mu_B/`/ion), SI (math:`\mathrm{Am}^2`/mol) or cgs (emu/mol) units.
It is a part of crystal field computation in Mantid and under active development. 
More documentation will follow as the development progresses.

Theory
------

The magnetisation is the expectation value of the magnetic moment operator :math:`\mathbf{\mu} = g_J \mathbf{J}`:

.. math:: M(H) = \frac{1}{Z} \sum_n \langle V_n(H) | g_J \mathbf{J} | V_n(H) \rangle \exp(-\beta E_n(H))

where :math:`H` is the magnetic field magnitude in Tesla, and :math:`g_J` is the Landé g-factor. The moment
operator is defined as :math:`\mathbf{J} = \hat{J}_x H_x + \hat{J}_y H_y + \hat{J}_z H_z` where :math:`\hat{J}_x`, 
:math:`\hat{J}_y`, and :math:`\hat{J}_z` are the angular momentum operators in Cartesian coordinates, with :math:`z` defined to 
be along the quantisation axis of the crystal fied (which is usually defined to be the highest symmetry rotation axis). 
:math:`H_x`, :math:`H_y`, and :math:`H_z` are the components of the unit vector pointing in the direction of the applied magnetic
field in this coordinate system. :math:`V_n(H)` and :math:`E_n(H)` are the n\ :sup:`th` eigenvector and 
eigenvalue (wavefunction and energy) obtained by diagonlising the Hamiltonian:

.. math:: \mathcal{H} = \mathcal{H}_{\mathrm{cf}} + \mathcal{H}_{\mathrm{Zeeman}} = \sum_{k,q} B_k^q \hat{O}_k^q 
   - g_J \mathbf{J}\cdot\mathbf{H}

where in this case the magnetic field :math:`\mathbf{H}` is not normalised. Finally, :math:`\beta = 1/(k_B T)` with :math:`k_B`
the Boltzmann constant and :math:`T` the temperature, and :math:`Z` is the partition sum :math:`Z = \sum_n \exp(-\beta E_n(H))`.

Example
-------

Here is an example of how to fit function's parameters to a spectrum. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself.

The `x`-axis is given in Tesla, and the magnetisation (`y`-axis) is in bohr magnetons per magnetic ion (:math:`\mu_B`/ion).

.. code::

    import numpy as np

    # Build a reference data set
    fun = 'name=CrystalFieldMagnetisation,Ion=Ce,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
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
