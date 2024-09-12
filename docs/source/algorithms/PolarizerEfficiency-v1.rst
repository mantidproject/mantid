
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates how the efficiency of a polarizer varies with wavelength. The
ordering of the workspaces in ``InputWorkspace`` is taken from the ``SpinStates`` parameter, and the analyser
efficiency, :math:`\epsilon_{cell}`, is given by ``AnalyserEfficiency``.

The polarization of the polarizer, :math:`P_{SM}`, is given by [#KRYCKA]_

.. math::
    P_{SM} = \frac{T_{00} - T_{01}}{2P_{cell}(T_{00} + T_{01})}

Since the efficiency, :math:`\epsilon_{SM}`, is given by :math:`\frac{1 + P_{SM}}{2}`, we have that

.. math::
    \epsilon_{SM} = \frac{1}{2} + \frac{T_{00} - T_{01}}{4(2\epsilon_{cell} - 1)(T_{00} + T_{01})}

The error in the calculation can then be determined thus:


.. math::
    \sigma_{\epsilon_{SM}} = \sqrt{|\frac{\delta \epsilon_{SM}}{\delta T_{00}}|^2 * \sigma^2_{T_{00}} + |\frac{\delta \epsilon_{SM}}{\delta T_{01}}|^2 * \sigma^2_{T_{01}} + |\frac{\delta \epsilon_{SM}}{\delta \epsilon_{cell}}|^2 * \sigma^2_{\epsilon_{cell}}}


where:

.. math::
    \frac{\delta \epsilon_{SM}}{\delta T_{00}} = \frac{T_{01}}{2(2\epsilon_{cell} - 1)(T_{00} + T_{01})^2}

.. math::
    \frac{\delta \epsilon_{SM}}{\delta T_{01}} = \frac{-T_{00}}{2(2\epsilon_{cell} - 1)(T_{00} + T_{01})^2}

.. math::
    \frac{\delta \epsilon_{SM}}{\delta \epsilon_{cell}} = \frac{T_{01} - T_{00}}{2(2\epsilon_{cell} - 1)^2(T_{00} + T_{01})}

Usage
-----

**Example - Calculate Polarizer Efficiency**

.. testcode:: PolarizerEfficiencyExample

    wsPara = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1-0.1))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1', NumBanks='1', BankPixelWidth='1')
    wsPara1 = CloneWorkspace(wsPara)
    wsAnti = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1+0.1))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1', NumBanks='1', BankPixelWidth='1')
    wsAnti1 = CloneWorkspace(wsAnti)

    grp = GroupWorkspaces([wsPara,wsAnti,wsPara1,wsAnti1])
    eCell = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=(1 + tanh(0.0733 * 12 * x * 0.2))/2',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1', NumBanks='1', BankPixelWidth='1')

    psm = PolarizerEfficiency(grp, eCell)
    print("Polarizer efficiency at a wavelength of " + str(mtd['psm'].dataX(0)[3]) + " Å is " + str(mtd['psm'].dataY(0)[3]))

Output:

.. testoutput:: PolarizerEfficiencyExample
    :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Polarizer efficiency at a wavelength of 4.0 Å is ...

References
----------

.. [#KRYCKA] Polarization-analyzed small-angle neutron scattering. I. Polarized data reduction using Pol-Corr, Kathryn Krycka et al, *Journal of Applied Crystallography*, **45** (2012), 546-553
          `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_


.. categories::

.. sourcelink::
