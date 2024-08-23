.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a pair of monitor-normalised, single-spectra workspaces representing a depolarized helium cell and the empty cell to
calculate the transmission of the depolarized cell, as described by Wildes [#WILDES]_ and by Krycka et al. [#KRYCKA]_.

.. math::

    T(\lambda) = T_E(\lambda) * exp(-\mu) = T_E(\lambda) * exp(-0.0733 * p * d * \lambda)


We first normalise the depolarised workspace :math:`T(\lambda)` by the empty cell workspace :math:`T_E(\lambda)`,
accounting for the neutrons lost to the glass cell and only considering the helium inside:

.. math::

    \frac{T(\lambda)}{T_E(\lambda)} = exp(-\mu) = exp(-0.0733 * p * d * \lambda)


We can then determine the cell path length multiplied by the gas pressure :math:`p * d` by using an exponential fit to
the curve of :math:`exp(-0.0733 * p * d * \lambda)`. The parameters table is then output, allowing for :math:`p * d`
(``PxD``) to be used in further corrections. Optionally, the calculated fit curves can also be output. See
:ref:`algm-Fit` for more details.

A polarised He\ :sub:`3`\  cell decays over time. At the end of its life, the cell is be actively depolarized and a run
is created to find the depolarized transmission rate through the helium. This allows for more effective efficiency
corrections.

When depolarized, :math:`P_{He} = 0`, allowing the transmission to be be determined using the above equations.


Usage
-----

**Example - Calculate Transmission**

.. testcode:: ExDepolTransmissionCalc

   # Create example workspaces.
   CreateSampleWorkspace(OutputWorkspace='mt', Function='User Defined', UserDefinedFunction='name=LinearBackground, A0=-0.112, A1=-0.004397', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)
   CreateSampleWorkspace(OutputWorkspace='dep', Function='User Defined', UserDefinedFunction='name=ExpDecay, Height=0.1239, Lifetime=1.338', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)

   output = DepolarizedAnalyserTransmission("dep", "mt")

   print("PXD Value = " + str(output.column("Value")[0]) + ".")

Output:

.. testoutput:: ExDepolTransmissionCalc
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

   PXD Value = ...


References
----------

.. [#WILDES] A. R. Wildes, *Neutron News*, **17** 17 (2006)
             `doi: 10.1080/10448630600668738 <https://doi.org/10.1080/10448630600668738>`_
.. [#KRYCKA] K. Krycka et al., *J. Appl. Crystallogr.*, **45** (2012)
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_


.. categories::

.. sourcelink::
