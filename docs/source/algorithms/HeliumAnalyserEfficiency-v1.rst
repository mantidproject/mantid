.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a normalised group workspace with four periods representing a run, combines this with the transmission of the empty
cell (``TransmissionEmptyCell``) and the cell path length multiplied by the gas pressure (``GasPressureTimesCellLength``) to calculate the efficiency of the analyser.
Optionally it will also output the value of :math:`p_{He}` in ``HeliumPolarization``, the transmission curves for
the parallel (``T_para``) or anti-parallel (``T_anti``) cases, or the analyser polarization (``AnalyserPolarization``).

The parameters ``TransmissionEmptyCell`` and ``GasPressureTimesCellLength`` are going to be calculated by fitting to data, and the covariance matrix of these
two parameters can be provided in order to calculate the errors on the transmission curves.

If the transmission of the wanted spin state is :math:`T_{para}`, and the transmission of the unwanted spin state is :math:`T_{anti}`,
then the polarization of of an unpolarised incoming beam after the analyser cell is given by

.. math::
    P_{cell} = \frac{T_{para} - T_{anti}}{T_{para} + T_{anti}} = \tanh(0.0733 p d \lambda p_{He})

If our four periods are :math:`T_{00}, T_{01}, T_{10}, T_{11}`, with the subscript denoting the spin configuration, then
:math:`T_{para} = T_{00} + T_{11}` and :math:`T_{anti} = T_{01} + T_{10}`, and we can calculate :math:`P_{cell}` from the above equation.
The value of :math:`pd` is given by the input ``pxd``, and :math:`\lambda` is the wavelength of each bin, so we fit
:math:`\tanh(0.0733 p d \lambda p_{He})` to our calculated :math:`P_{cell}` to give us :math:`p_{He}`.

The efficiencies of the analyser for parallel and antiparallel neutrons are then given by

.. math::
    \epsilon_{para} = \frac{1 + P_{cell}}{2}

    \epsilon_{anti} = \frac{1 - P_{cell}}{2}

Since the polarization of the analyser is the same for up and down neutrons, these two efficiencies define all four combinations of
spin states, i.e. :math:`\epsilon_{00} = \epsilon_{11}` and :math:`\epsilon_{10} = \epsilon_{01}` [#KRYCKA]_.

We can then also calculate the theoretical transmission curves :math:`T_{para}` and :math:`T_{anti}` from the equations

.. math::
    T_{para} = \frac{T_E}{2}\exp(-0.0733 p d \lambda (1 - p_{He}))

    T_{anti} = \frac{T_E}{2}\exp(-0.0733 p d \lambda (1 + p_{He}))

To calculate the errors :math:`\Delta T_{para}` and :math:`\Delta T_{anti}` we need the error on :math:`p_{He}`, :math:`\Delta p_{He}`, and
the covariance matrix from the calculation of :math:`T_E` and :math:`pd`. If :math:`\mathbf{\partial T}` is the 3x1 vector of partial
derivatives of :math:`T_{para}` with respect to each parameter, and :math:`S` is the 3x3 parameter covariance matrix, then
:math:`\Delta T_{para}` (and similarly for :math:`\Delta T_{anti}`) is given by

.. math::
    \frac{\Delta T_{para}}{t_{crit}} = \sqrt{\mathbf{\partial T}^T S \mathbf{\partial T}}

If :math:`n_b` is the number of histogram bins used in the fit, then define :math:`n := n_b-3`, since we are fitting three parameters. Then the
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
    efficiencies = HeliumAnalyserEfficiency(grp, SpinStates='11,01,00,10', HeliumAtomsPolarization='p_He', AnalyserPolarization='P')

    print('P at ' + str(mtd['P'].dataX(0)[0]) + 'A = ' + str(mtd['P'].dataY(0)[0]))
    print('Error in PA at ' + str(mtd['P'].dataX(0)[0]) + 'A = ' + str(mtd['P'].dataE(0)[0]))
    print('p_He Value = ' + str(mtd['p_He'].dataY(0)[0]))
    print('Parallel efficiency at ' + str(mtd['efficiencies11'].dataX(0)[0]) + 'A = ' + str(mtd['efficiencies11'].dataY(0)[0]))

Output:

.. testoutput:: ExHeliumAnalyserEfficiencyCalc

    PA at 2.0A = 0.962520839134
    Error in PA at 2.0A = 2.84935838704
    p_He Value = 0.900000000409
    Parallel efficiency at 2.0A = 0.981260419567

References
----------

.. [#KRYCKA] Polarization-analyzed small-angle neutron scattering. I. Polarized data reduction using Pol-Corr, Kathryn Krycka et al, *Journal of Applied Crystallography*, **45** (2012), 546-553
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_


.. categories::

.. sourcelink::