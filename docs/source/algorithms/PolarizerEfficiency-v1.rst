
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates how the efficiency of a polarizer varies with wavelength. The
ordering of the workspaces in ``InputWorkspace`` is taken from the ``SpinStates`` parameter, and the analyser
efficiency, :math:`\epsilon_{cell}`, is given by ``AnalyserEfficiency``.

The polarization of the polarizer, :math:`P_{SM}`, is given by

.. math::
    P_{SM} = \frac{T_{00} - T_{01}}{2P_{cell}}

Since the efficiency, :math:`\epsilon_{SM}`, is given by :math:`\frac{1 + P_{SM}}{2}`, we have that

.. math::
    \epsilon_{SM} = \frac{1}{2} + \frac{T_{00} - T_{01}}{8\epsilon_{cell} - 4}

Usage
-----

**Example - Calculate Polarizer Efficiency**

.. testcode:: PolarizerEfficiencyExample

    wsPara = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1-0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsPara1 = CloneWorkspace(wsPara)
    wsAnti = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=0.5*exp(-0.0733*12*x*(1+0.9))',XUnit='Wavelength', xMin='1',XMax='8', BinWidth='1')
    wsAnti1 = CloneWorkspace(wsAnti)

    grp = GroupWorkspaces([wsPara,wsAnti,wsPara1,wsAnti1])
    eCell = CreateSampleWorkspace('Histogram', Function='User Defined', UserDefinedFunction='name=UserFunction,Formula=(1 + tanh(0.0733 * 12 * x * 0.2))/2',XUnit='Wavelength', xMin='1',XMax='16', BinWidth='1')

    psm = PolarizerEfficiency(grp, eCell)
    print("Polarizer efficiency at a wavelength of " + str(mtd['psm'].dataX(0)[3]) + " Å is " + str(mtd['psm'].dataY(0)[3]))

Output:

.. testoutput:: PolarizerEfficiencyExample
    :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    Polarizer efficiency at a wavelength of 4.0 Å is ...

.. categories::

.. sourcelink::
