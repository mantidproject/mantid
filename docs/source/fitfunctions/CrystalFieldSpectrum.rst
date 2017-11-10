.. _func-CrystalFieldSpectrum:

====================
CrystalFieldSpectrum
====================

.. index:: CrystalFieldSpectrum

Description
-----------

This function calculates a spectrum of a crystal electric field acting upon a rare earth ion. It is a part of crystal field computation
in Mantid and under active development. More documentation will follow as the development progresses.

Here is an example of how to fit function's parameters to a spectrum. All parameters disallowed by symmetry are fixed automatically.
Any other parameters that need fixing has to be tied explicitly. Peak centres and intensities are also fixed and computed from the
field parameters with the :ref:`CrystalFieldPeaks <func-CrystalFieldPeaks>` function. Any other peak parameter can be set using
the "f-index-dot-name" syntax (see :ref:`CompositeFunction <func-CompositeFunction>` for more details).

The `x`-axis is given in meV, and the intensity (`y`-axis) is in milibarn per steradian per meV.

.. code::

    import numpy as np

    # Build a reference data set
    fun = 'name=CrystalFieldSpectrum,Ion=Ce,Temperature=44,ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,B40=-0.031787,B42=-0.11611,B44=-0.12544,'
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

   Ion;String;Mandatory;An element name for a rare earth ion. Possible values are: Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb.
   Symmetry;String;C1;A symbol for a symmetry group. Setting `Symmetry` automatically zeros and fixes all forbidden parameters. Possible values are: C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3, S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh
   Temperature;Double;1.0;A temperature in Kelvin.
   ToleranceEnergy;Double;:math:`10^{-10}`;Tolerance in energy in meV. If difference between two or more energy levels is smaller than this value they are considered degenerate.
   ToleranceIntensity;Double;:math:`10^{-3}`;Tolerance in intensity. If difference between intensities of two or more transitions is smaller than this value the transitions are considered degenerate.
   PeakShape;String;Lorentzian;A name of a function (peak type) to describe the shape of each peak. Currently Lorentzian (default) and Gaussian sre supported.
   FWHM;Double;0.0;The default full peak width at half maximum. If not set explicitly via function parameters the peaks will have this width (not fixed).
   
If the peak functions have any own attributes they can be accessed using the "f-index-dot-name" syntax (see :ref:`CompositeFunction <func-CompositeFunction>` for more details).


.. properties::

.. categories::

.. sourcelink::
