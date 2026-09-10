.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm locates the specular line in a reflectometry detector workspace. It integrates the selected spectra over the :math:`X` axis, optionally restricted by *RangeLower* and *RangeUpper*, to produce a detector profile whose :math:`X` values are workspace indices from the input workspace.

*LineCentre* is the fractional **workspace index** associated with the specular peak. It is not a spectrum number or detector ID. For example, a value of 42.3 places the fitted line centre between the spectra stored at workspace indices 42 and 43, irrespective of their spectrum numbers or detector IDs. The value is returned directly in *LineCentre*.

The algorithm uses the maximum of the profile as the initial line centre and estimates its width from the half-height points. It then fits a Gaussian together with the background selected by *BackgroundType* over the interval ``initial line centre ± FitWindowMultiplier × FWHM``. A ``success`` status from :ref:`Fit <algm-Fit>` is always accepted. The tolerance-limited statuses indicating that changes in the function or parameter values became too small are accepted by default and can be controlled independently using *AcceptChangesInFunctionTooSmall* and *AcceptChangesInParameterTooSmall*. When the fit status is accepted, *LineCentre* contains the optimized Gaussian centre.

By default, if a fit cannot be completed or reports an unsuccessful status, *LineCentre* contains the initial profile maximum and *OutputStatus* reports that the initial centre was used. If *UseFittedLineCentreOnFailure* is true and the fit completes with a finite, in-range line centre, that fitted centre is returned regardless of the fit status and *OutputStatus* contains the status reported by :ref:`Fit <algm-Fit>`. If the fit throws, the initial centre is still returned. The algorithm raises an error if no finite initial line centre can be found. *OutputProfileWorkspace* and *OutputFitWorkspace* are optional; the fit workspace is only produced when a fitted centre is returned.

Previous Versions
-----------------

Version 3 returns the line position through the scalar *LineCentre* property and does not provide the optional single-valued *OutputWorkspace* from version 2. Callers that require the version 2 API can select it explicitly, for example ``FindReflectometryLines(InputWorkspace="ws", OutputWorkspace="position", Version=2)``. See :ref:`FindReflectometryLines-v2 <algm-FindReflectometryLines-v2>` for its documentation.

Usage
-----

**Example - Find a specular line with a flat background**

.. testcode:: FindReflectometryLinesV3Example

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

   result = FindReflectometryLines(
       InputWorkspace="detector_workspace",
       BackgroundType="Flat",
       OutputProfileWorkspace="detector_profile",
       OutputFitWorkspace="peak_fit",
   )
   print(f"Line centre: {result.LineCentre:.1f}")
   print(f"Fit status: {result.OutputStatus}")

Output:

.. testoutput:: FindReflectometryLinesV3Example

   Line centre: 10.3
   Fit status: success

.. categories::

.. sourcelink::
