.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a normalised group workspace with four periods representing a run, combines this with the cell path length multiplied by the
gas pressure (``GasPressureTimesCellLength``) to calculate the efficiency of the analyser.

If the transmission of the wanted spin state is :math:`T_{para}`, and the transmission of the unwanted spin state is :math:`T_{anti}`,
then the polarization of of an unpolarised incoming beam after the analyser cell, :math:`P_{cell}` is given by [#KRYCKA]_

.. math::
    P_{cell} = \frac{T_{para} - T_{anti}}{T_{para} + T_{anti}} = \tanh(0.0733 p d \lambda p_{He})

The efficiency of the analyser cell is given by

.. math::
    \epsilon_{cell} = \frac{1 + P_{cell}}{2} = \frac{T_{para}}{T_{para} + T_{anti}} = \frac{1 + \tanh(0.0733 p d \lambda p_{He})}{2}

If our four periods are :math:`T_{00}, T_{01}, T_{10}, T_{11}`, with the subscript denoting the spin configuration, then
:math:`T_{para} = T_{00} + T_{11}` and :math:`T_{anti} = T_{01} + T_{10}`, and we can calculate :math:`\epsilon_{cell}` from the above equation.
The value of :math:`pd` is given by the input ``GasPressureTimesCellLength``, and :math:`\lambda` is the wavelength of each bin, so we fit
:math:`\frac{1 + \tanh(0.0733 p d \lambda p_{He})}{2}` to our calculated :math:`\epsilon_{cell}` to give us :math:`p_{He}` and hence the
theoretical efficiency curve.

To calculate the error, :math:`\Delta \epsilon_{cell}`, we need the error on :math:`p_{He}`, :math:`\Delta p_{He}`, and
the error of :math:`pd`, :math:`\Delta pd`, given by ``GasPressureTimesCellLengthError``. The covariance between :math:`pd` and :math:`p_{He}`
is assumed to be zero. Then the error :math:`\Delta \epsilon_{cell}` is given by

.. math::
    \frac{\Delta \epsilon_{cell}}{t_{crit}} = \sqrt{\left(\frac{\partial \epsilon_{cell}}{\partial p_{He}} \Delta p_{He}\right)^2 + \left(\frac{\partial \epsilon_{cell}}{\partial pd}\Delta pd \right)^2}

If :math:`n_b` is the number of histogram bins used in the fit, then define :math:`n := n_b-2`, since we are fitting two parameters (:math:`pd` and :math:`p_{He}`). Then the
factor :math:`t_{crit}` follows a :math:`t` distribution with :math:`n` degrees of freedom, and probability density function :math:`f_t(x,n)`.
For a standard 68.3% (1-sigma) error the factor :math:`t_{crit}` is given by the solution to

.. math::
	\frac{1}{2}\left(1 + \mathrm{erf}\left(\frac{1}{\sqrt{2}}\right)\right) = P(X < t_{crit} ) = \int_0^{t_{crit}} f_t(x; n) dx

As the number of histogram bins used in the fit increases, :math:`t_{crit} \rightarrow 1`.

Usage
-----

**Example - Calculate Analyser Efficiency**

.. testcode:: ExHeliumAnalyserEfficiencyCalc

    wsPara = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1-0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsPara1 = CloneWorkspace(wsPara)
    wsAnti = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1+0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsAnti1 = CloneWorkspace(wsAnti)

    grp = GroupWorkspaces([wsPara,wsAnti,wsPara1,wsAnti1])
    eff = HeliumAnalyserEfficiency(grp, SpinStates='11,01,00,10')

    print('Efficiency at ' + str(mtd['eff'].dataX(0)[0]) + ' Å = ' + str(mtd['eff'].dataY(0)[0]))
    print('Error in efficiency at ' + str(mtd['eff'].dataX(0)[0]) + ' Å = ' + str(mtd['eff'].dataE(0)[0]))

Output:

.. testoutput:: ExHeliumAnalyserEfficiencyCalc
    :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Efficiency at 1.0 Å = ...
    Error in efficiency at 1.0 Å = ...

References
----------

.. [#KRYCKA] Polarization-analyzed small-angle neutron scattering. I. Polarized data reduction using Pol-Corr, Kathryn Krycka et al, *Journal of Applied Crystallography*, **45** (2012), 546-553
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_


.. categories::

.. sourcelink::