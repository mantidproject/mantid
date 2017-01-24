.. _func-CrystalFieldSusceptibility:

==========================
CrystalFieldSusceptibility
==========================

.. index:: CrystalFieldSusceptibility

Description
-----------

This function calculates the crystal field contribution to the *molar* magnetic susceptibility using the Van Vleck formula. 
The function outputs the results by default in *cgs* units of cm\ :sup:`3`\ /mol == "emu/mol". 
There are also options to output the result in SI (m\ :sup:`3`\ /mol) or "atomic" units (\ :math:`\mu_B`/Tesla/ion).

Theory
------

The magnetic susceptibility can be calculated by treating the magnetic field (Zeeman interaction) as a perturbation on the crystal 
field energy. To second order, the susceptibility per mole of magnetic ion is given by:

.. math:: \chi(T) = \frac{N_A}{Z} \sum_n \left[ \frac{| \langle V_n | g_J \mu_B \mathbf{J} | V_n \rangle | ^2}{k_B T} 
   - 2 \sum_{m \neq n} \frac{| \langle V_n | g_J \mu_B \mathbf{J} | V_m \rangle | ^2}{E_n - E_m} \right] \exp(-\beta E_n)

where :math:`N_A` is Avogadro's constant, :math:`k_B` is Boltzmann's constant, :math:`Z` is the partition sum, and 
:math:`V_n` and :math:`E_n` are the n-th wavefunction (eigenvector) and energy level (eigenvalue) of the unperturbed 
crystal field Hamiltonian. :math:`g_J` is the Landé g-factor, :math:`\mu_B` is the Bohr magneton and the moment operator 
is defined as :math:`\mathbf{J} = \hat{J}_x B_x + \hat{J}_y B_y + \hat{J}_z B_z` where :math:`\hat{J}_x`, :math:`\hat{J}_y`,
and :math:`\hat{J}_z` are the angular momentum operators in Cartesian coordinates, with :math:`z` defined to 
be along the quantisation axis of the crystal field (which is usually defined to be the highest symmetry rotation axis). 
:math:`B_x`, :math:`B_y`, and :math:`B_z` are the components of the unit vector pointing in the direction of the applied 
magnetic field in this coordinate system.

Finally, in order to account for the effect of any exchange interactions in the system which will shift the susceptibility curve
up or down (analogous to the Curie-Weiss temperature), the actual magnetic susceptibility calculated by this function is:

.. math:: \chi^{\mathrm{eff}} = \frac{\chi(T)}{1 - \lambda \chi(T)}

where :math:`\lambda` parameterises an effective exchange interaction and :math:`\chi` is the bare (paramagnetic Crystal Field)
susceptibility. A negative :math:`\lambda` indicates overall antiferromagnetic interactions, whilst a positive :math:`\lambda`
corresponds to overall ferromagnetic interactions.

Example
-------

Here is an example of how to the crystal field parameters to a susceptibility dataset. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself, for a field along the [111] direction with respects to the crystal field parameters (not necessarily the [111] crystallographic direction).

The `x`-axis is given in Kelvin, and the susceptibility (`y`-axis) is in cgs units of cm\ :sup:`3`\ /mol (==emu/mol).

.. testcode:: ExampleCrystalFieldSusceptibility

    import numpy as np

    # Build a reference data set
    fun = 'name=CrystalFieldSusceptibility,Ion=Ce,B20=0.37737,B22=0.039770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
    fun += 'Hdir=(1,1,1), Unit=cgs, inverse=1,'
    
    # This creates a (empty) workspace to use with EvaluateFunction
    x = np.linspace(1, 300, 300)
    y = x * 0
    e = y + 1
    ws = CreateWorkspace(x, y, e)
    
    # The calculated data will be in 'data', WorkspaceIndex=1
    EvaluateFunction(fun, ws, OutputWorkspace='data')
    
     # Change parameters slightly and fit to the reference data
    fun = 'name=CrystalFieldSusceptibility,Ion=Ce,Symmetry=C2v,B20=0.4,B22=0.04,B40=-0.03,B42=-0.1,B44=-0.1,'
    fun += 'Hdir=(1,1,1), Unit=cgs, inverse=1,'
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

.. testcleanup:: ExampleCrystalFieldSusceptibility

.. testoutput:: ExampleCrystalFieldSusceptibility
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

        B20 =  0.37737
        B22 =  0.039788
        B40 = -0.031787
        B42 = -0.11611
        B44 = -0.12544
    Cost function value =  1.0921e-14

.. attributes::

   Ion;String;Mandatory;An element name for a rare earth ion. Possible values are: Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb.
   Symmetry;String;C1;A symbol for a symmetry group. Setting `Symmetry` automatically zeros and fixes all forbidden parameters. Possible values are: C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3, S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh
   powder;Boolean;false; Whether to calculate the powder averaged magnetisation or not.
   Hdir;Vector;(0.,0.,1.); The direction of the applied field w.r.t. the crystal field parameters
   Unit;String;'bohr'; The desired units of the output, either: 'bohr' (muB/T/ion), 'SI' (m^3/mol) or 'cgs' (cm^3/mol).
   inverse;Boolean;false; Whether to output 1/chi(T) instead of chi(T).

.. properties::

.. categories::

.. sourcelink::
