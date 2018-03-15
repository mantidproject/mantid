.. _func-PrimStretchedExpFT:

==================
PrimStretchedExpFT
==================

.. index:: PrimStretchedExpFT

.. attributes::

.. properties::

Description
-----------

Provides the Fourier Transform of the Symmetrized Stretched Exponential Function integrated over
each energy bin.

.. math:: S(Q,E) = \int_{E-\delta E}^{E+\delta E} dE' StretchedExpFT(E', Centre, Tau, Beta)

with :math:`StretchedExpFT` is fit function `StretchedExpFT <func-StretchedExpFT>`__.
Quantity :math:`\delta E` is evaluated at run time. If we request to evaluate this
function over a set of energy values :math:`E_0,..,E_N`,
then :math:`2\delta E = \frac{E_N-E_0}{N}`

In the picture below we evaluated function StretcheExpFT at the center of bins of width
:math:`0.4 \mu eV` (label <i>center_evaluated</i>). However, one should compare the integral
of ​the model within each bin against experimental QENS data, because the data is a histogram.
Nevertheless, we usually compare the value that the model takes at the center of each bin instead
of the integral. This approximation is acceptable when the model does not change much
within each bin. This is specially true at high energies when StretcheExpFT is very smooth.
Sometimes this center-of-the-bin evaluation is wrong, like in the central bin [-0.2, 0.2]ueV.
The integral of :math:`StretchedExpFT` in this bin is the red dot, and the value of
:math:`StretchedExpFT` at the center of the bin is the black dot at the top. Very different!
This situation is likely to arise for samples with a relaxation rate :math:`h/Tau \ll \delta E`.
Finally, the blue line is the evaluation of :math:`PrimStretchedExpFT` at each bin center, which
by design amounts to the integral of :math:`StretchedExpFT` within each bin. This curve
coincides with the red dot for the central bin, as expected. Also, it agrees with the
center-of-bin evaluation of :math:`StretchedExpFT` at high energies, when the center-of-the-bin
approximation is acceptable.

.. figure:: /images/PrimStretchedExpFT_fig1.png
   :alt: Comparison of Pseudo-Voigt function with Gaussian and Lorentzian profiles.

.. _PrimStretchedExpFT-usage:

Usage
-----
.. include:: ../../usagedata-note.txt

**Example - Fit to a QENS signal:**

The QENS signal is modeled by the convolution of a resolution function
with elastic and PrimStretchedExpFT components.
Noise is modeled by a linear background:

:math:`S(Q,E) = R(Q,E) \otimes (\alpha \delta(E) + PrimStretchedExpFT(Q,E)) + (a+bE)`

Obtaining an initial guess close to the optimal fit is critical. For this model,
it is recommended to follow these steps:
- In the Fit Function window of MantidPlot, construct the model.
- Tie parameter :math:`Height` of PrimStretchedExpFT to zero, then carry out the Fit. This will result in optimized elastic line and background.
- Untie parameter :math:`Height` of PrimStretchedExpFT and tie parameter :math:`Beta` to 1.0, then carry out the fit. This will result in optimized model using an exponential.
- Release the tie on Beta  and redo the fit.

.. testcode:: ExamplePrimStretchedExpFT

   # Load resolution function and scattered signal
   resolution = LoadNexus(Filename="resolution_14955.nxs")
   qens_data = LoadNexus(Filename="qens_data_14955.nxs")

   # This function_string is obtained by constructing the model
   # with the Fit Function window of MantidPlot, then
   # Setup--> Manage Setup --> Copy to Clipboard
   function_string  = "(composite=Convolution,FixResolution=true,NumDeriv=true;"
   function_string += "name=TabulatedFunction,Workspace=resolution,WorkspaceIndex=0,Scaling=1,Shift=0,XScaling=1;"
   function_string += "(name=DeltaFunction,Height=1,Centre=0;"
   function_string += "name=PrimStretchedExpFT,Height=1.0,Tau=100,Beta=0.98,Centre=0));"
   function_string += "name=LinearBackground,A0=0,A1=0"

   # Carry out the fit. Produces workspaces  fit_results_Parameters,
   #  fit_results_Workspace, and fit_results_NormalisedCovarianceMatrix.
   Fit(Function=function_string,
      InputWorkspace="qens_data",
      WorkspaceIndex=0,
      StartX=-0.15, EndX=0.15,
      CreateOutput=1,
      Output="fit_results")

   # Collect and print parameters for PrimStrechtedExpFT
   parameters_of_interest = ("Tau", "Beta")
   values_found = {}
   ws = mtd["fit_results_Parameters"]  # Workspace containing optimized parameters
   for row_index in range(ws.rowCount()):
      full_parameter_name = ws.row(row_index)["Name"]
      for parameter in parameters_of_interest:
         if parameter in full_parameter_name:
            values_found[parameter] = ws.row(row_index)["Value"]
            break
   if values_found["Beta"] > 0.63 and values_found["Beta"] < 0.71:
      print("Beta found within [0.63, 0.71]")
   if values_found["Tau"] > 54.0 and values_found["Tau"] < 60.0:
      print( "Tau found within [54.0, 60.0]")

.. testcleanup:: ExamplePrimStretchedExpFT

   DeleteWorkspace("resolution")
   DeleteWorkspace("qens_data")
   DeleteWorkspace("fit_results_Parameters")
   DeleteWorkspace("fit_results_Workspace")
   DeleteWorkspace("fit_results_NormalisedCovarianceMatrix")

Output:

.. testoutput:: ExamplePrimStretchedExpFT

   Beta found within [0.63, 0.71]
   Tau found within [54.0, 60.0]

.. categories::

.. sourcelink::
   :py: Framework/PythonInterface/plugins/functions/PrimStretchedExpFT.py
