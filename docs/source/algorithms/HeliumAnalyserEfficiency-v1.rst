.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a normalised group workspace with four periods representing a run, combines this with the transmission of the empty
cell (``T_E``) and the cell path length multiplied by the gas pressure (``pxd``) to calculate the efficiency of the analyser.
It will output the calculated efficiency in ``p_He``, as well as the transmission curves for the parallel (``T_para``), and
anti-parallel (``T_anti``) cases.

The parameters ``T_E`` and ``pxd`` are going to be calculated by fitting to data, and the covariance matrix of these
two parameters can be provided in order to calculate the errors on the transmission curves.

If the transmission of the wanted spin state is :math:`T_{para}`, and the transmission of the unwanted spin state is :math:`T_{anti}`,
then the polarisation of of an unpolarised incoming beam after the analyser cell is given by

.. math::
    P_A = \frac{T_{para} - T_{anti}}{T_{para} + T_{anti}} = \tanh(-0.0733 p d \lambda p_{He})

If our four periods are :math:`T_{00}, T_{01}, T_{10}, T_{11}`, with the subscript denoting the spin configuration, then
:math:`T_{para} = T_{00} + T_{11}` and :math:`T_{anti} = T_{01} + T_{10}`, and we can calculate :math:`P_A` from the above equation.
The value of :math:`pd` is given by the input ``pxd``, and :math:`\lambda` is the wavelength of each bin, so we fit
:math:`\tanh(-0.0733 p d \lambda p_{He})` to our calculated :math:`P_A` to give us :math:`p_{He}`.

We can then calculate the theoretical transmission curves :math:`T_{para}` and :math:`T_{anti}` from the equations

.. math::
    T_{para} = \frac{T_E}{2}\exp(-0.0733 p d \lambda (1 - p_{He}))

    T_{anti} = \frac{T_E}{2}\exp(-0.0733 p d \lambda (1 + p_{He}))

To calculate the errors :math:`\Delta T_{para}` and :math:`\Delta T_{anti}` we need the error on :math:`p_{He}`, :math:`\Delta p_{He}`, and
the covariance matrix from the calculation of :math:`T_E` and :math:`pd`, given by :math:`S = [\sigma_{ij}]`, where
:math:`\sigma_{00} = \left( \Delta T_E \right)^2` and :math:`\sigma_{11} = \left( \Delta pd \right)^2`, with the diagonal terms given by the
covariances. Then :math:`\Delta T_{para}` (and similarly for :math:`\Delta T_{anti}`) is given by

.. math::
    \frac{\Delta T_{para}}{\Delta t} = \sqrt{\frac{\partial T_{para}}{\partial p_{He}}^2 \left(\Delta p_{He} \right)^2 + \frac{\partial T_{para}}{\partial T_E}^2 \sigma_{00} + \frac{\partial T_{para}}{\partial T_E}\frac{\partial T_{para}}{\partial pd}\left(\sigma_{01} + \sigma_{10}\right) +  \frac{\partial T_{para}}{\partial pd}^2 \sigma_{11}}

If :math:`n_b` is the number of histogram bins used in the fit, then define :math:`n := n_b-3`, since we are fitting three parameters. Then the
factor :math:`\Delta t` follows a :math:`t` distribution with :math:`n` degrees of freedom, and probability density function :math:`f_t(x,n)`.
For a standard 68.3% (1-sigma) error the factor :math:`\Delta t` is given by the solution to

.. math::
	\frac{1}{2}\left(1 + \mathrm{erf}\left(\frac{1}{\sqrt{2}}\right)\right) = P(X < \Delta t ) = \int_0^{\Delta t} f_t(x; n) dx

As the number of histogram bins used in the fit increases, :math:`\Delta t \rightarrow 1`.

Usage
-----

**Example - Calculate Analyser Efficiency**

.. testcode:: ExHeliumAnalyserEfficiencyCalc

    wsPara = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1-0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsPara1 = CloneWorkspace(wsPara)
    wsAnti = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1+0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsAnti1 = CloneWorkspace(wsAnti)

    grp = GroupWorkspaces([wsPara,wsAnti,wsPara1,wsAnti1])
    HeliumAnalyserEfficiency(grp, SpinConfigurations='11,01,00,10')

    print("p_He Value = " + str(mtd["p_He"].dataY(0)[0]) + ".")

Output:

.. testoutput:: ExHeliumAnalyserEfficiencyCalc
   :hide:
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

   p_He Value = ...

.. categories::

.. sourcelink::