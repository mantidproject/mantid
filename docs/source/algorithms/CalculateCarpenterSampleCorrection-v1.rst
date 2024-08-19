.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm is a port to C++ of a multiple scattering absorption
correction, used to correct the vanadium spectrum for the GPPD
instrument at the IPNS. The correction calculation was originally worked
out by Jack Carpenter and Asfia Huq and implemented in Java by Alok
Chatterjee. The java code was translated to C++ in Mantid by Dennis
Mikkelson.

* Elastic scattering is assumed

In [1]_ we see that the calculation of the attenuation factor F involves
an integral over the sample cylinder. By expanding the integrands as a power series,
we can factor out any dependence on scattering cross section and radius.
These integral terms are denoted by :math:`Z_{mn}` and so we may write:

.. math::
   \frac{1}{F} = \sum_{m=0}^\infty\sum_{n=0}^\infty\frac{(-1)^{m+n}}{m!n!}(\mu R)^{m+n} Z_{mn}(\theta)

where :math:`\mu` is the inverse scattering length.

The functions :math:`Z_{mn}(\theta)` are written in terms of Chebyshev
expansion coefficients:

.. math::
  Z_{mn}(\theta) = \sum_{s=0}^\infty c_{s}(m,n)cos(s\theta)

where the Chebyshev coefficients :math:`c_{s}(m,n)` up to  m + n
:math:`\leqslant` 5 have been tabulated and are stored as an array by the algorithm.

This version of the correction follows the implementation in [1]_ in that it only calculates for the correction in-plane, unlike [2]_, [3]_ that generalizes the correction to out-of-plane.

This algorithm calculates and outputs the absorption and/or multiple scattering correction workspaces to be applied to the InputWorkspace. Thus, there are, at most, two workspaces in the OutputWorkspaceBaseName group workspace. This allows for flexibility of applying either correction to a workspace without having to apply both (as is the case with :ref:`algm-CarpenterSampleCorrection`). For the case where both corrections are calculated, the output will be the following:

1. The absorption correction workspace will be OutputWorkspaceBaseName + `_abs` and will be in `.getItem(0)`.

2. The multiple scattering correction workspace will be OutputWorkspaceBaseName + `_ms` and will be in `.getItem(1)`.

This is the child algorithm that :ref:`algm-CarpenterSampleCorrection` (previously known as *MultipleScatteringCylinderAbsorption*) uses to calculate and apply the correction to a sample workspace.

Usage
-----

**Example: Calculate corrections for a simple cylindrical sample**

.. testcode:: ExCalculateCarpenterSampleCorrection_corrections

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    #restrict the number of wavelength points to speed up the example
    wsOut = CalculateCarpenterSampleCorrection(ws,CylinderSampleRadius=0.2)

    print("Absorption Correction Output:  {}".format(wsOut.getItem(0).readY(0)))
    print("Multiply Scattering Correction Output:  {}".format(wsOut.getItem(1).readY(0)))

Output:

.. testoutput:: ExCalculateCarpenterSampleCorrection_corrections

    Absorption Correction Output:  [0.85283805 0.79620318 0.74348494 0.69440412 0.64870017 0.62121997]
    Multiply Scattering Correction Output:  [0.09633662 0.09991619 0.1034959  0.10705826 0.11058382 0.11280196]

To reproduce what :ref:`algm-CarpenterSampleCorrection` does, you can calculate and apply the correction as follows

**Example: Apply correction for a simple cylindrical sample using getItem**

.. testcode:: ExCalculateCarpenterSampleCorrection_apply1

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    corrections = CalculateCarpenterSampleCorrection(ws,CylinderSampleRadius=0.2)

    # Get absorption correction
    absCorr = corrections.getItem(0)

    # Get multiple scattering correction
    msFactor  = corrections.getItem(1)
    msCorr = Multiply(ws, msFactor)

    # Apply absorption correction to workspace
    ws_abs_corrected = Divide(ws, absCorr)

    # Apply multiple scattering correction to workspace
    ws_ms_corrected = Minus(ws, msCorr)

    # Apply both corrections
    wsOut = Minus(ws_abs_corrected, msCorr)

    print("Absorption Corrected Output:  {}".format(ws_abs_corrected.readY(0)))
    print("Multiple Scattering Corrected Output:  {}".format(ws_ms_corrected.readY(0)))
    print("Combined Corrected Output:  {}".format(wsOut.readY(0)))

Output:

.. testoutput:: ExCalculateCarpenterSampleCorrection_apply1

    Absorption Corrected Output:  [ 6.66892661  7.14329517 21.0999759   8.1904963   8.76755487  2.51509668]
    Multiple Scattering Corrected Output:  [ 5.13959844  5.11923959 14.06392099  5.07861898  5.05856725  1.38618331]
    Combined Corrected Output:  [ 6.1210107   6.57502041 19.47638255  7.58160094  8.13860778  2.33885171]

**Example: Apply correction for a simple cylindrical sample using getItem**

.. testcode:: ExCalculateCarpenterSampleCorrection_apply2

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    #restrict the number of wavelength points to speed up the example
    basename = "corrections"
    CalculateCarpenterSampleCorrection(ws,OutputWorkspaceBaseName=basename,
                                       CylinderSampleRadius=0.2)

    # Get absorption correction
    absCorr = mtd[basename+"_abs"]

    # Get multiple scattering correction
    msFactor = mtd[basename+"_ms"]
    msCorr = Multiply(ws, msFactor)

    # Apply absorption correction to workspace
    ws_abs_corrected = Divide(ws, absCorr)

    # Apply multiple scattering correction to workspace
    ws_ms_corrected = Minus(ws, msCorr)

    # Apply both corrections
    wsOut = Minus(ws_abs_corrected, msCorr)

    print("Absorption Corrected Output:  {}".format(ws_abs_corrected.readY(0)))
    print("Multiple Scattering Corrected Output:  {}".format(ws_ms_corrected.readY(0)))
    print("Combined Corrected Output:  {}".format(wsOut.readY(0)))

Output:

.. testoutput:: ExCalculateCarpenterSampleCorrection_apply2

    Absorption Corrected Output:  [ 6.66892661  7.14329517 21.0999759   8.1904963   8.76755487  2.51509668]
    Multiple Scattering Corrected Output:  [ 5.13959844  5.11923959 14.06392099  5.07861898  5.05856725  1.38618331]
    Combined Corrected Output:  [ 6.1210107   6.57502041 19.47638255  7.58160094  8.13860778  2.33885171]

References
----------

.. [1] J.M. Carpenter *Attenuation Correction Factor for Scattering from Cylindrical Targets* Review of Scientific Instruments **40.4** (1969): 555. doi: `10.1063/1.1684003 <http://dx.doi.org/10.1063/1.1684003>`_

.. [2] D.F.R. Mildner, J.M. Carpenter, and C.A. Pelizzari *Generalized Attenuation Correction Factor for Scattering from Cylindrical Targets* Review of Scientific Instruments **45.4** (1974): 572. doi: `10.1063/1.1686687 <http://dx.doi.org/10.1063/1.1686687>`_

.. [3] D.F.R. Mildner and J.M.Carpenter *Improvements to the Chebyshev Expansion of Attenuation Correction Factors for Cylindrical Samples.* J Appl Crystallogr **23.5** (1990): 378–386 doi: `10.1107/S0021889890005258 <http://dx.doi.org/10.1107/S0021889890005258>`_

.. categories::

.. sourcelink::
