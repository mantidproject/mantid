.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It
   might get changed or even removed without a notification, should
   instrument scientists decide to do so.

Performs a Time-of-flight (TOF) to dSpacing conversion using
calibrated pixel positions, focuses the values in dSpacing (summing
them up into a single spectrum) and then converts them back to
TOF. The output workspace produced by this algorithm has one spectrum.

If a table of detector positions is passed as an input property, the
detectors are calibrated before performing the conversions between TOF
and dSpacing.

In any case, before focusing the workspace, the spectra are corrected
by using data from a Vanadium run (passed in the VanadiumWorkspace
property). These corrections include two steps: detector sensitivity
correction and pixel-by-pixel correction on a per-bank basis. See also
:ref:`algm-EnggCalibrateFull` where the same correction is applied.

The pixel-by-pixel Vanadium correction is based on curves fitted for
every bank. These curves can be obtained via the output property
OutVanadiumCurveFits. For every bank, the curve is fitted to the
summation of all the spectra of the bank, and then every spectra of
the bank is divided by the bank curve.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Simple focussing of an EnginX data file:**

.. testcode:: ExSimpleFocussing

   # Run the algorithm on an EnginX file
   data_ws = Load('ENGINX00213855.nxs')

   # Using precalculated Vanadium corrections. To calculate from scrach see EnggVanadiumCorrections
   van_integ_ws = Load('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   van_curves_ws = Load('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')

   focussed_ws = EnggFocus(InputWorkspace=data_ws,
                           VanIntegrationWorkspace=van_integ_ws,
                           VanCurvesWorkspace=van_curves_ws,
                           Bank='1')

   # Should have one spectrum only
   print "No. of spectra:", focussed_ws.getNumberHistograms()

   # Print a few arbitrary bins where higher intensities are expected
   fmt = "For TOF of {0:.3f} intensity is {1:.3f}"
   for bin in [3169, 6037, 7124]:
     print fmt.format(focussed_ws.readX(0)[bin], focussed_ws.readY(0)[bin])

.. testcleanup:: ExSimpleFocussing

   DeleteWorkspace(focussed_ws)
   DeleteWorkspace(data_ws)
   DeleteWorkspace(van_integ_ws)
   DeleteWorkspace(van_curves_ws)

Output:

.. testoutput:: ExSimpleFocussing

   No. of spectra: 1
   For TOF of 20165.642 intensity is 13.102
   For TOF of 33547.826 intensity is 17.844
   For TOF of 38619.804 intensity is 32.768
   
.. categories::

.. sourcelink::
