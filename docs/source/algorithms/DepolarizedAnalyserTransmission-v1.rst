.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a pair of normalised, single-spectra workspaces representing a depolarised helium cell and the empty cell. It will
then determine the empty cell transmission value, ``T_E``, and the cell path length multiplied by the gas pressure
``pxd`` by using an exponential fit. The calculated fit curve and parameters table are then output for use in later
calculations. See :ref:`algm-Fit` for more details.

A polarised He\ :sub:`3`\  cell decays over time. At the end of its life, it will be fully depolarised and a run is
created to find the depolarised transmission rate through the helium. This allows for more effective efficiency
corrections.

When depolarised, :math:`P_{He} = 0`, therefore the transmission can be determined using
:math:`T(\lambda) = T_E(\lambda) * exp(-\mu) = T_E(\lambda) * exp(-0.0733 * p * d * \lambda)`. We can then use this
equation, after normalising the ``DepolarisedWorkspace`` by the ``EmptyCellWorkspace``, to perform a fit to determine
our :math:`T_E` (``T_E``) and :math:`p * d` (``pxd``) values.


Usage
-----

**Example - Calculate Transmission**

.. testcode:: ExDepolTransmissionCalc

   # Create example workspaces.
   CreateSampleWorkspace(OutputWorkspace='mt', Function='User Defined', UserDefinedFunction='name=UserFunction, Formula=1.465e-07*exp(0.0733*4.76*x)', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)
   CreateSampleWorkspace(OutputWorkspace='dep', Function='User Defined', UserDefinedFunction='name=UserFunction, Formula=0.0121*exp(-0.0733*10.226*x)', XUnit='wavelength', NumBanks=1, BankPixelWidth=1, XMin=3.5, XMax=16.5, BinWidth=0.1)

   output = DepolarizedAnalyserTransmission("dep", "mt", OutputParameters="output_params")

   print("PXD Value = " + str(mtd[output_params].column("Value")[0]) + ".")
   print("T_E Value = " + str(mtd[output_params].column("Value")[1]) + ".")

Output:

.. testoutput:: ExDepolTransmissionCalc
   :hide:
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

   PXD Value = ...
   T_E Value = ...

.. categories::

.. sourcelink::
