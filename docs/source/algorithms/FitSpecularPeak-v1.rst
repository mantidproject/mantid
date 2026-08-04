.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm locates the specular peak in a reflectometry detector workspace. It integrates the selected spectra over the :math:`X` axis, optionally restricted by *RangeLower* and *RangeUpper*, to produce a detector profile whose :math:`X` values are workspace indices from the input workspace.

*PeakCentre* is the fractional **workspace index** associated with the specular peak. It is not a spectrum number or detector ID. For example, a value of 42.3 places the fitted peak centre between the spectra stored at workspace indices 42 and 43, irrespective of their spectrum numbers or detector IDs.

The algorithm uses the maximum of the profile as the initial peak centre and estimates its width from the half-height points. It then fits a Gaussian together with the background selected by *BackgroundType*. When the fit succeeds, *PeakCentre* contains the optimized Gaussian centre and *PeakCentreError* contains its uncertainty.

If a fit cannot be completed, *PeakCentre* contains the initial profile maximum and *OutputStatus* reports that the initial centre was used. The algorithm raises an error if no finite initial peak centre can be found. *OutputProfileWorkspace* and *OutputFitWorkspace* are optional; the fit workspace is only produced for a successful fit.

Usage
-----

**Example - Fit a specular peak with a flat background**

.. testcode:: FitSpecularPeakExample

   import math

   profile = [2.0 + 20.0 * math.exp(-0.5 * ((index - 10.3) / 2.0) ** 2) for index in range(21)]
   CreateWorkspace(
       DataX=[0.0, 1.0] * len(profile),
       DataY=profile,
       DataE=[1.0] * len(profile),
       NSpec=len(profile),
       UnitX="Wavelength",
       OutputWorkspace="detector_workspace",
   )

   result = FitSpecularPeak(
       InputWorkspace="detector_workspace",
       BackgroundType="Flat",
       OutputProfileWorkspace="detector_profile",
       OutputFitWorkspace="peak_fit",
   )
   print(f"Peak centre: {result.PeakCentre:.1f}")
   print(f"Fit status: {result.OutputStatus}")

Output:

.. testoutput:: FitSpecularPeakExample

   Peak centre: 10.3
   Fit status: success

.. categories::

.. sourcelink::
