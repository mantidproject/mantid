.. _func-CrystalFieldMagnetisation:

=========================
CrystalFieldMagnetisation
=========================

.. index:: CrystalFieldMagnetisation

Description
-----------

This function calculates the crystal field (molar) *magnetic moment* as a function of applied magnetic field in a specified 
direction, in either atomic (:math:`\mu_B/`/ion), SI (Am\ :sup:`2`\ /mol) or cgs (erg/Gauss/mol == emu/mol) units. 
If using cgs units, the magnetic field (:math:`x`-axis) is expected to be in Gauss. If using SI or atomic units, the field 
should be given in Tesla.

Strictly, to obtain the *magnetisation*, one should divide by the molar volume of the material.

Theory
------

The function calculates the expectation value of the magnetic moment operator :math:`\mathbf{\mu} = g_J \mu_B \mathbf{J}`:

.. math:: M(B) = \frac{1}{Z} \sum_n \langle V_n(H) | g_J \mu_B \mathbf{J} | V_n(H) \rangle \exp(-\beta E_n(H))

where :math:`B` is the magnetic field in Tesla, :math:`g_J` is the Landé g-factor, :math:`\mu_B` is the Bohr magneton. 
The moment operator is defined as :math:`\mathbf{J} = \hat{J}_x B_x + \hat{J}_y B_y + \hat{J}_z B_z` where 
:math:`\hat{J}_x`, :math:`\hat{J}_y`, and :math:`\hat{J}_z` are the angular momentum operators in Cartesian coordinates, 
with :math:`z` defined to be along the quantisation axis of the crystal field (which is usually defined to be the highest 
symmetry rotation axis). :math:`B_x`, :math:`B_y`, and :math:`B_z` are the components of the unit vector pointing in the 
direction of the applied magnetic field in this coordinate system. :math:`V_n(B)` and :math:`E_n(B)` are the n\ :sup:`th` 
eigenvector and eigenvalue (wavefunction and energy) obtained by diagonalising the Hamiltonian:

.. math:: \mathcal{H} = \mathcal{H}_{\mathrm{cf}} + \mathcal{H}_{\mathrm{Zeeman}} = \sum_{k,q} B_k^q \hat{O}_k^q 
   - g_J \mu_B \mathbf{J}\cdot\mathbf{B}

where in this case the magnetic field :math:`\mathbf{B}` is not normalised. Finally, :math:`\beta = 1/(k_B T)` 
with :math:`k_B` the Boltzmann constant and :math:`T` the temperature, and :math:`Z` is the partition sum 
:math:`Z = \sum_n \exp(-\beta E_n(H))`.

Example
-------

Here is an example of how to fit crystal field parameters to a magnetisation measurement. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself.

The `x`-axis is given in Tesla, and the magnetisation (`y`-axis) is in bohr magnetons per magnetic ion (:math:`\mu_B`/ion).

.. testcode:: ExampleCrystalFieldMagnetisation

    import numpy as np
    
    # Build a reference data set
    fun = 'name=CrystalFieldMagnetisation,Ion=Ce,B20=0.37737,B22=0.039770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
    fun += 'Temperature=10'
    
    # This creates a (empty) workspace to use with EvaluateFunction
    x = np.linspace(0, 30, 300)
    y = x * 0
    e = y + 1
    ws = CreateWorkspace(x, y, e)
    
    # The calculated data will be in 'data', WorkspaceIndex=1
    EvaluateFunction(fun, ws, OutputWorkspace='data')
    
     # Change parameters slightly and fit to the reference data
    fun = 'name=CrystalFieldMagnetisation,Ion=Ce,Symmetry=C2v,Temperature=10,B20=0.4,B22=0.04,B40=-0.03,B42=-0.1,B44=-0.1,'
    fun += 'ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0)'
    
    # (set MaxIterations=0 to see the starting point)
    Fit(fun, 'data', WorkspaceIndex=1, Output='fit',MaxIterations=100, CostFunction='Unweighted least squares')
    # Using Unweighted least squares fit because the data has no errors.

    # Extract fitted parameters
    parws = mtd['fit_Parameters']
    for i in range(parws.rowCount()):
        row = parws.row(i)
        if row['Value'] != 0:
            print "%7s = % 7.5g" % (row['Name'], row['Value'])

.. testcleanup:: ExampleCrystalFieldMagnetisation

.. testoutput:: ExampleCrystalFieldMagnetisation
   :hide:
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

        B20 =  0...
        B22 =  0...
        B40 = -0...
        B42 = -0...
        B44 = -0...
    Cost function value = ...

Output (the numbers you see on your machine may vary):

.. code::

        B20 =  0.39541
        B22 =  0.030001
        B40 = -0.029841
        B42 = -0.11611
        B44 = -0.1481
    Cost function value =  1.2987e-14

.. attributes::

   Ion;String;Mandatory;An element name for a rare earth ion. Possible values are: Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb.
   Symmetry;String;C1;A symbol for a symmetry group. Setting `Symmetry` automatically zeros and fixes all forbidden parameters. Possible values are: C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3, S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh
   Temperature;Double;1.0; Temperature in Kelvin of the measurement.
   powder;Boolean;false; Whether to calculate the powder averaged magnetisation or not.
   Hdir;Vector;(0.,0.,1.); The direction of the applied field w.r.t. the crystal field parameters
   Unit;String;'bohr'; The desired units of the output, either: 'bohr' (muB/ion), 'SI' (Am^2/mol) or 'cgs' (erg/G/mol).
   
.. properties::

.. categories::

.. sourcelink::
