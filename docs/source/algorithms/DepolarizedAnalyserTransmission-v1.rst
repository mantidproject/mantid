.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a pair of normalised, single-spectra workspaces representing a depolarized helium cell and the empty cell. It will
then determine the empty cell transmission value, ``T_E``, and the cell path length multiplied by the gas pressure
``pxd`` by using an exponential fit. The parameters table is then output for use in later calculations. Optionally, the
calculated fit curve and a non-normalised version of the covariance matrix can also be output to check the quality of
the fit. See :ref:`algm-Fit` for more details.

A polarised He\ :sub:`3`\  cell decays over time. At the end of its life, it will be fully depolarized and a run is
created to find the depolarized transmission rate through the helium. This allows for more effective efficiency
corrections.

When depolarized, :math:`P_{He} = 0`, therefore the transmission can be determined using
:math:`T(\lambda) = T_E(\lambda) * exp(-\mu) = T_E(\lambda) * exp(-0.0733 * p * d * \lambda)`. We can then use this
equation, after normalising the ``DepolarizedWorkspace`` by the ``EmptyCellWorkspace``, to perform a fit to determine
our :math:`T_E` (``T_E``) and :math:`p * d` (``pxd``) values.


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
