.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a pair of monitor-normalised, single-spectra workspaces representing a depolarized helium cell and the empty cell to
calculate the transmission of the depolarized cell.

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
   CreateSampleWorkspace(OutputWorkspace='mt', Function='User Defined', UserDefinedFunction='name=UserFunction, Formula=1.465e-07*exp(0.0733*4.76*x)', XUnit='wavelength', NumMonitors=1, NumBanks=0, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)
   CreateSampleWorkspace(OutputWorkspace='dep', Function='User Defined', UserDefinedFunction='name=UserFunction, Formula=0.0121*exp(-0.0733*10.226*x)', XUnit='wavelength', NumMonitors=1, NumBanks=0, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)

   output = DepolarizedAnalyserTransmission("dep", "mt")

   print("PXD Value = " + str(output.column("Value")[0]) + ".")
   print("T_E Value = " + str(output.column("Value")[1]) + ".")

Output:

.. testoutput:: ExDepolTransmissionCalc
   :hide:
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

   PXD Value = ...
   T_E Value = ...

.. categories::

.. sourcelink::
