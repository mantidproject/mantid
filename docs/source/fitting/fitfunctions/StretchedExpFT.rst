.. _func-StretchedExpFT:

==============
StretchedExpFT
==============

.. index:: StretchedExpFT

.. attributes::

.. properties::

Description
-----------

Provides the Fourier Transform of the Symmetrized Stretched Exponential Function

.. math:: S(Q,E) = Height \int_{-\infty}^{\infty} dt/h \cdot e^{-i2\pi (E-Centre)t/h} \cdot e^{-|\frac{t}{Tau}|^{Beta}} )

with :math:`h` Planck's constant. If the energy units of energy are micro-eV, then tau is expressed in pico-seconds. If E-units are micro-eV then
tau is expressed in nano-seconds.

Properties:

- Normalization :math:`\int_{-\infty}^{\infty} dE \cdot S(Q,E) = Height`
- Maximum :math:`S(Q,E\equiv 0)=Height \cdot Tau \cdot Beta^{-1} \cdot \Gamma(Beta^{-1})`

.. _StretchedExpFT-usage:

Usage
-----
.. include:: ../../usagedata-note.txt

**Example - Fit to a QENS signal:**

The QENS signal is modeled by the convolution of a resolution function
with elastic and StretchedExpFT components.
Noise is modeled by a linear background:

:math:`S(Q,E) = R(Q,E) \otimes (\alpha \delta(E) + StretchedExpFT(Q,E)) + (a+bE)`

Obtaining an initial guess close to the optimal fit is critical. For this model, it is recommended to follow these steps:
- In the Fit Function window of MantidPlot, construct the model.
- Tie parameter :math:`Height` of StretchedExpFT to zero, then carry out the Fit. This will result in optimized elastic line and background.
- Untie parameter :math:`Height` of StretchedExpFT and tie parameter :math:`Beta` to 1.0, then carry out the fit. This will result in optimized model using an exponential.
- Release the tie on Beta  and redo the fit.

.. testcode:: ExampleStretchedExpFT

   # Load resolution function and scattered signal
   resolution = LoadNexus(Filename="resolution_14955.nxs")
   qens_data = LoadNexus(Filename="qens_data_14955.nxs")

   # This function_string is obtained by constructing the model
   # with the Fit Function window of MantidPlot, then
   # Setup--> Manage Setup --> Copy to Clipboard
   function_string  = "(composite=Convolution,FixResolution=true,NumDeriv=true;"
   function_string += "name=TabulatedFunction,Workspace=resolution,WorkspaceIndex=0,Scaling=1,Shift=0,XScaling=1;"
   function_string += "(name=DeltaFunction,Height=1,Centre=0;"
   function_string += "name=StretchedExpFT,Height=1.0,Tau=100,Beta=0.98,Centre=0));"
   function_string += "name=LinearBackground,A0=0,A1=0"

   # Carry out the fit. Produces workspaces  fit_results_Parameters,
   #  fit_results_Workspace, and fit_results_NormalisedCovarianceMatrix.
   Fit(Function=function_string,
      InputWorkspace="qens_data",
      WorkspaceIndex=0,
      StartX=-0.15, EndX=0.15,
      CreateOutput=1,
      Output="fit_results")

   # Collect and print parameters for StrechtedExpFT
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
      print("Tau found within [54.0, 60.0]")

.. testcleanup:: ExampleStretchedExpFT

   DeleteWorkspace("resolution")
   DeleteWorkspace("qens_data")
   DeleteWorkspace("fit_results_Parameters")
   DeleteWorkspace("fit_results_Workspace")
   DeleteWorkspace("fit_results_NormalisedCovarianceMatrix")

Output:

.. testoutput:: ExampleStretchedExpFT

   Beta found within [0.63, 0.71]
   Tau found within [54.0, 60.0]

.. categories::

.. sourcelink::
   :py: Framework/PythonInterface/plugins/functions/StretchedExpFT.py
