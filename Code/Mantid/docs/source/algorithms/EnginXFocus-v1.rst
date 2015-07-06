.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

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
correction and pixel-by-pixel correction on a per-bank basis.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Simple focussing of and EnginX data file:**

.. testcode:: ExSimpleFocussing

   # Run the algorithm
   ws_name = 'data_ws'
   Load('ENGINX00213855.nxs', OutputWorkspace=ws_name)
   ws = EnginXFocus(InputWorkspace=ws_name, Bank='1')

   # Should have one spectrum only
   print "No. of spectra:", ws.getNumberHistograms()

   # Print a few arbitrary bins where higher intensities are expected
   fmt = "For TOF of {0:.3f} intensity is {1:.3f}"
   for bin in [3169, 6037, 7124]:
     print fmt.format(ws.readX(0)[bin], ws.readY(0)[bin])

.. testcleanup:: ExSimpleFocussing

   DeleteWorkspace(ws_name)

Output:

.. testoutput:: ExSimpleFocussing

   No. of spectra: 1
   For TOF of 20165.642 intensity is 7.436
   For TOF of 33547.826 intensity is 9.388
   For TOF of 38619.804 intensity is 17.397
   
.. categories::

.. sourcelink::
