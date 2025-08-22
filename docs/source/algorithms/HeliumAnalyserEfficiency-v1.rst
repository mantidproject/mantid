.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a set of normalised polarized transmission runs as group workspaces. For each transmission run:

- Calculates the analyzer cell efficiency (:math:`\epsilon_{cell}`) by looking at the polarization from the wanted  :math:`T_{para}` and unwanted  :math:`T_{anti}` spin states
- Extracts the polarization of the Helium gas (:math:`p_{He}`) from a fit to : :math:`\epsilon_{cell}=\frac{1}{2}\big(1+\tanh(\mu p_{He})\big)`
- Extract the timestamp at which the transmission measurement was realized

Once polarizations and timestamps for all the input transmission runs are retrieved, the decay parameter (:math:`\Gamma`) and initial polarization (:math:`p_{He_{0}}`)
are extracted from a fit to: :math:`p_{He}(t)=p_{He_{0}}(t) e^{-t/\Gamma}`

Each of the input workspaces must be a group of four single spectrum histogram workspaces, each
representing the transmission of a known spin state as specified by the ``SpinStates`` property.

Efficiency and Polarization of Helium gas
-----------------------------------------

If the transmission of the wanted spin state is :math:`T_{para}`, and the transmission of the unwanted spin state is :math:`T_{anti}`,
then the polarization of an unpolarized incoming beam after the analyser cell, :math:`P_{cell}` is given by [#KRYCKA]_

.. math::
    P_{cell} = \frac{T_{para} - T_{anti}}{T_{para} + T_{anti}} = \tanh(\mu p_{He})

The efficiency of the analyser cell is given by

.. math::
    \epsilon_{cell} = \frac{1 + P_{cell}}{2} = \frac{T_{para}}{T_{para} + T_{anti}} = \frac{1 + \tanh(\mu \,p_{He})}{2}

Where the polarization is :math:`p_{He}` and :math:`\mu` is the opacity of the cell, defined as : :math:`\mu=0.0733pd\lambda`. Where
:math:`pd` is the mean gas length that can be computed in :ref:`algm-DepolarizedAnalyserTransmission` algorithm, and :math:`\lambda` is the wavelength of each bin.

With the four periods in each input workspace being :math:`T_{00}, T_{01}, T_{10}, T_{11}`, and the subscript denoting the spin configuration, then
:math:`T_{para} = T_{00} + T_{11}` and :math:`T_{anti} = T_{01} + T_{10}`, and we can calculate :math:`\epsilon_{cell}` from the above equation.
We fit :math:`\frac{1 + \tanh(0.0733pd\lambda p_{He})}{2}` to our calculated :math:`\epsilon_{cell}` to give us :math:`p_{He}`.

To calculate the error, :math:`\sigma_{\epsilon_{cell}}`, we need the error on :math:`p_{He}`, :math:`\sigma_{p_{He}}`, and
the error of :math:`pd`, :math:`\sigma_{pd}`, given by ``PXDError``. The covariance between :math:`pd` and :math:`p_{He}`
is assumed to be zero. Then the error :math:`\sigma_{\epsilon_{cell}}` is given by

.. math::
    \frac{\sigma_{\epsilon_{cell}}}{t_{crit}} = \sqrt{\left(\frac{\partial \epsilon_{cell}}{\partial p_{He}} \sigma_{p_{He}}\right)^2 + \left(\frac{\partial \epsilon_{cell}}{\partial mu}\sigma_{mu}\right)^2 + \left(\frac{\partial \epsilon_{cell}}{\partial \lambda}\sigma_{\lambda}\right)^2}

where :math:`mu = 0.0733 * pd` and :math:`\sigma_{\lambda}` is the width of the wavelength bin.

If :math:`n_b` is the number of histogram bins used in the fit, then define :math:`n := n_b-1`, since we are fitting one parameter (:math:`p_{He}`). Then the
factor :math:`t_{crit}` follows a :math:`t` distribution with :math:`n` degrees of freedom, and probability density function :math:`f_t(x,n)`.
For a standard 68.3% (1-sigma) error the factor :math:`t_{crit}` is given by the solution to

.. math::
	\frac{1}{2}\left(1 + \mathrm{erf}\left(\frac{1}{\sqrt{2}}\right)\right) = P(X < t_{crit} ) = \int_0^{t_{crit}} f_t(x; n) dx

As the number of histogram bins used in the fit increases, :math:`t_{crit} \rightarrow 1`.

Helium Polarization Decay
-------------------------

After the polarizations are extracted from the transmission data {:math:`p_{He}`}, the timestamp at which each run occurred as well as the difference
in time between the input runs is extracted using the :ref:`algm-TimeDifference-v1` algorithm, then the polarizations and times are fit
to an exponential decay curve:

.. math::
    p_{He}(t) = p_{He_{0}}(t) e^{-t/\Gamma}

From this fit, the decay time :math:`\Gamma` and initial polarization :math:`p_{He_{0}}` are obtained. These parameters can be used
in :ref:`algm-HeliumAnalyserEfficiency` algorithm to retrieve the efficiency of the fitted helium cell at an arbitrary time.

Outputs
-------

Up to three distinct outputs can be retrieved from this algorithm:

- OutputWorkspace: Efficiencies. A group workspace of MatrixWorkspaces with the calculated analytical efficiencies for each input transmission run.
- OutputCurves: A group workspace of MatrixWorkspaces with the fitted curves from both the polarization of helium fit (suffix `He3_Polarization_curves`) and the decay in polarization fit(suffix `decay_curves`).
- OutputParameters: A TableWorkspace with the parameters from both the polarization of helium fit and the decay in polarization fit.

If only one input group is set on the `InputWorkspaces` property, the algorithm will not do a Polarization Decay fit, and the output will only be the efficiency of the group
and the results from the Helium Gas Polarization fit.

Usage
-----

**Example - Calculate Analyser Polarization Decay**

.. testcode:: ExHeliumPolDecay

    from mantid.api import mtd
    from mantid.kernel import DateAndTime
    import numpy as np

    def createWorkspaceWithTime(x,y,name,delay):
         CreateWorkspace(DataX = x, DataY = y, OutputWorkspace = name, UnitX = 'Wavelength')
         run = mtd[name].getRun()
         start = np.datetime64("2025-07-01T08:00:00")
         run.setStartAndEndTime(DateAndTime(str(start+int(3600*delay))), DateAndTime(str(start+int(3600*(delay+1)))))
         ConvertToHistogram(InputWorkspace = name, OutputWorkspace = name)

    groups = []
    tau = 45
    polIni = 0.6
    delays = np.linspace(0,20,3)
    lam = np.linspace(1.75,8,10)
    mu = 0.0733 * 12 * lam
    phe = polIni * np.exp(-delays/tau)

    for n, delay in enumerate(delays):
        ynsf = np.exp(- mu * (1-phe[n]))
        ysf =  np.exp(-mu * (1 + phe[n]))
        names = [name + f"_{n}" for name in ["T00", "T11", "T01", "T10"]]
        [createWorkspaceWithTime(lam, ynsf, name, delay) for name in names[:2]]
        [createWorkspaceWithTime(lam, ysf, name, delay) for name in names[2:]]
        groups.append(f"group_{n}")
        GroupWorkspaces(InputWorkspaces = names, OutputWorkspace = groups[-1])

    out, curves, table = HeliumAnalyserEfficiency(InputWorkspaces=groups,
                                SpinStates = "00,11,01,10")

    p1 = mtd['table'].column(1)[0:3]
    p0, tau = mtd['table'].column(1)[4:-1]

    print(f"Polarizations at delay times are {p1[0]:.2f}, {p1[1]:.2f}, {p1[2]:.2f}")
    print(f"Initial He3 Polarization is {p0:.2f}")
    print(f"Polarization decay time is {tau:.2f}")

Output:

.. testoutput:: ExHeliumPolDecay

    Polarizations at delay times are 0.60, 0.48, 0.38
    Initial He3 Polarization is 0.60
    Polarization decay time is 45.00


References
----------

.. [#KRYCKA] Polarization-analyzed small-angle neutron scattering. I. Polarized data reduction using Pol-Corr, Kathryn Krycka et al, *Journal of Applied Crystallography*, **45** (2012), 546-553
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_


.. categories::

.. sourcelink::
