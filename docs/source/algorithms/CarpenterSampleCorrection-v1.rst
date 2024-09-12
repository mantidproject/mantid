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

This algorithm calls :ref:`algm-CalculateCarpenterSampleCorrection` to calculate both absorption and multiple scattering corrections and then applies both to the sample workspace.

Usage
-----

**Example: A simple cylindrical sample**

.. testcode:: ExCarpenterSampleCorrection

    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,"Wavelength")
    ws = Rebin(ws,Params=[1])
    SetSampleMaterial(ws,ChemicalFormula="V")

    #restrict the number of wavelength points to speed up the example
    wsOut = CarpenterSampleCorrection(ws,CylinderSampleRadius=0.2)

    print("Output:  {}".format(wsOut.readY(0)))

Output:

.. testoutput:: ExCarpenterSampleCorrection

    Output:  [ 6.1210107   6.57502041 19.47638255  7.58160094  8.13860778  2.33885171]


References
----------

.. [1] J.M. Carpenter *Attenuation Correction Factor for Scattering from Cylindrical Targets* Review of Scientific Instruments **40.4** (1969): 555. doi: `10.1063/1.1684003 <http://dx.doi.org/10.1063/1.1684003>`_

.. [2] D.F.R. Mildner, J.M. Carpenter, and C.A. Pelizzari *Generalized Attenuation Correction Factor for Scattering from Cylindrical Targets* Review of Scientific Instruments **45.4** (1974): 572. doi: `10.1063/1.1686687 <http://dx.doi.org/10.1063/1.1686687>`_

.. [3] D.F.R. Mildner and J.M.Carpenter *Improvements to the Chebyshev Expansion of Attenuation Correction Factors for Cylindrical Samples.* J Appl Crystallogr **23.5** (1990): 378–386 doi: `10.1107/S0021889890005258 <http://dx.doi.org/10.1107/S0021889890005258>`_

.. categories::

.. sourcelink::
