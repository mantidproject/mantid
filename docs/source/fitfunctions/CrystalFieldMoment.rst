.. _func-CrystalFieldMoment:

==================
CrystalFieldMoment
==================

.. index:: CrystalFieldMoment

Description
-----------

This function calculates the crystal field contribution to the magnetic moment as a function of temperature at a constant
applied magnetic field in a specified direction, in either atomic (:math:`\mu_B/`/ion), SI (Am\ :sup:`2`\ /mol) or 
cgs (erg/Gauss/mol == emu/mol) units. 

Theory
------

The magnetic moment is calculated as the thermal expectation value of the magnetic moment operator 
:math:`\mathbf{\mu} = g_J \mu_B \mathbf{J}`:

.. math:: M(B) = \frac{1}{Z} \sum_n \langle V_n(H) | g_J \mathbf{J} | V_n(H) \rangle \exp(-\beta E_n(H))

where :math:`B` is the magnetic field in Tesla, :math:`g_J` is the Landé g-factor, and :math:`\mu_B` is the Bohr magneton. 
The moment operator is defined as :math:`\mathbf{J} = \hat{J}_x B_x + \hat{J}_y B_y + \hat{J}_z B_z` where 
:math:`\hat{J}_x`, :math:`\hat{J}_y`, and :math:`\hat{J}_z` are the angular momentum operators in Cartesian coordinates, 
with :math:`z` defined to be along the quantisation axis of the crystal field (which is usually defined to be the highest 
symmetry rotation axis). :math:`B_x`, :math:`B_y`, and :math:`B_z` are the components of the unit vector pointing in the 
direction of the applied magnetic field in this coordinate system. :math:`V_n(B)` and :math:`E_n(B)` are the n\ :sup:`th` 
eigenvector and eigenvalue (wavefunction and energy) obtained by diagonalising the Hamiltonian:

.. math:: \mathcal{H} = \mathcal{H}_{\mathrm{cf}} + \mathcal{H}_{\mathrm{Zeeman}} = \sum_{k,q} B_k^q \hat{O}_k^q 
   - g_J \mu_B \mathbf{J}\cdot\mathbf{B}

where in this case the magnetic field :math:`\mathbf{B}` is not normalised. Finally, :math:`\beta = 1/(k_B T)` with 
:math:`k_B` the Boltzmann constant and :math:`T` the temperature, and :math:`Z` is the partition sum 
:math:`Z = \sum_n \exp(-\beta E_n(H))`.

Example
-------

Here is an example of how to fit M(T) to a measured dataset. All parameters disallowed by symmetry are fixed automatically.
The "data" here is generated from the function itself.

The `x`-axis is the temperature in Kelvin, and the magnetic moment (`y`-axis) is in Am\ :sup:`2`\ /mol (SI units), and the "measurement" was done with a field of 0.01 Tesla along the [110] direction of the crystal field (not necessarily the crystallographic [110] direction).

.. testcode:: ExampleCrystalFieldMoment

    import numpy as np
    
    # Build a reference data set
    fun = 'name=CrystalFieldMoment,Ion=Ce,B20=0.37737,B22=0.039770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
    fun += 'Hmag=0.01, Hdir=(1,1,0), Unit=SI,'
    
    # This creates a (empty) workspace to use with EvaluateFunction
    x = np.linspace(1, 300, 300)
    y = x * 0
    e = y + 1
    ws = CreateWorkspace(x, y, e)
    
    # The calculated data will be in 'data', WorkspaceIndex=1
    EvaluateFunction(fun, ws, OutputWorkspace='data')
    
     # Change parameters slightly and fit to the reference data
    fun = 'name=CrystalFieldMoment,Ion=Ce,Symmetry=C2v,B20=0.37,B22=0.04,B40=-0.032,B42=-0.12,B44=-0.13,'
    fun += 'Hmag=0.01, Hdir=(1,1,0), Unit=SI,'
    fun += 'ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0)'
    
    # (set MaxIterations=0 to see the starting point)
    Fit(fun, 'data', WorkspaceIndex=1, Output='fit',MaxIterations=100, CostFunction='Unweighted least squares')
    # Using Unweighted least squares fit because the data has no errors.

    # Extract fitted parameters
    parws = mtd['fit_Parameters']
    for i in range(parws.rowCount()):
        row = parws.row(i)
        if row['Value'] != 0:
            print("%7s = % 7.5g" % (row['Name'], row['Value']))

.. testcleanup:: ExampleCrystalFieldMoment

.. testoutput:: ExampleCrystalFieldMoment
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

        B20 =  0.37745
        B22 =  0.016732
        B40 = -0.032093
        B42 = -0.11298
        B44 = -0.12685
    Cost function value =  9.7067e-18

.. attributes::

   Ion;String;Mandatory;An element name for a rare earth ion. Possible values are: Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb.
   Symmetry;String;C1;A symbol for a symmetry group. Setting `Symmetry` automatically zeros and fixes all forbidden parameters. Possible values are: C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3, S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh
   powder;Boolean;false; Whether to calculate the powder averaged magnetisation or not.
   Hmag;Double;1.0; The applied magnetic field magnitude in Tesla (for 'bohr' or 'SI' units) or Gauss (for 'cgs' units).
   Hdir;Vector;(0.,0.,1.); The direction of the applied field w.r.t. the crystal field parameters
   Unit;String;'bohr'; The desired units of the output, either: 'bohr' (muB/ion), 'SI' (Am^2/mol) or 'cgs' (erg/G/mol).
   inverse;Boolean;false; Whether to output 1/M(T) instead of M(T).
   
.. properties::

.. categories::

.. sourcelink::
