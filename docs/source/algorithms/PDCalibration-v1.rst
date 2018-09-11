
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calibrates the detector pixels and creates a
:ref:`diffraction calibration workspace
<DiffractionCalibrationWorkspace>`. Unlike
:ref:`algm-CalibrateRectangularDetectors` the peak fitting and
calibration is done in TOF not d spacing. The peak d values are
converted to TOF based on either the old calibration or the instrument
geometry. The ``InputWorkspace`` contains the data from a standard
sample. The results are then fitted with up to (in order) difc, t_zero and difa,
these values are details in :ref:`algm-AlignDetectors`.

The peak fitting properties are explained in more detail in
:ref:`algm-FitPeaks`. This is used to perform a refinement of peaks
using as much information as is provided as possible. Each input
spectrum is calibrated separately following the same basic steps:

1. The ``PeakPositions`` are used to determine fit windows in combination with ``PeakWindow``. The windows are half the distance between the provided peak positions, with a maximum size of ``PeakWindow``.
2. The positions and windows are converted to time-of-flight for the spectrum using either the previous calibration information or the spectrum's geometry.
3. For each peak, the background is estimated from the first and last ten points in the fit window.
4. For each peak, the nominal center is selected by locating the highest point near the expected position. The height is used as the initial guess as well.
5. For each peak, the width is estimated by multiplying the peak center position with ``PeakWidthPercent`` or by calculating the second moment of the data in the window.
6. For each peak, the peak fit parameters are refined.
7. All of the fitted peak centers are used to fit the calibration constants, weighted by peak height.

If more than one constant is requested, the result that has the lowest
`reduced chi-squared value
<https://en.wikipedia.org/wiki/Reduced_chi-squared_statistic>`_ is
returned. This favors using less parameters.

A mask workspace is created, named "OutputCalibrationTable" + '_mask',
with uncalibrated pixels masked.

The resulting calibration table can be saved with
:ref:`algm-SaveDiffCal`, loaded with :ref:`algm-LoadDiffCal` and
applied to a workspace with :ref:`algm-AlignDetectors`. There are also
three workspaces placed in the ``DiagnosticWorkspace`` group. They are:

* evaluated fit functions (``_fitted``)
* raw peak fit values (``_fitparam``)
* contain the fitted positions in dspace( ``_dspacing``)
* peak widths (``_width``)
* peak heights (``_height``)
* instrument resolution (delta-d/d ``_resolution``)

Since multiple peak shapes can be used,
see the documentation for the individual `fit functions
<../fitfunctions/index.html>`_ to see how they relate to the effective
values displayed in the diagnostic tables. For ``Gaussian`` and
``Lorentzian``, the widths and resolution are converted to values that
can be directly compared with the results of
:ref:`algm-EstimateResolutionDiffraction`.

Usage
-----

**Example - PDCalibration**

.. code-block:: python

   # If you have a old calibration it can be used as the starting point
   oldCal = 'NOM_calibrate_d72460_2016_05_23.h5'

   # list of d values for diamond
   dvalues = (0.3117,0.3257,0.3499,0.4205,0.4645,0.4768,0.4996,0.5150,0.5441,0.5642,0.5947,0.6307,.6866,.7283,.8185,.8920,1.0758,1.2615,2.0599)

   LoadEventNexus(Filename='NOM_72460', OutputWorkspace='NOM_72460')
   PDCalibration(InputWorkspace='NOM_72460',
                 TofBinning=[300,-.001,16666.7],
                 PreviousCalibrationFile=oldCal,
                 PeakPositions=dvalues,
                 PeakWidthPercent=.008,
                 OutputCalibrationTable='cal',
                 DiagnosticWorkspaces='diag')

   # Print the result
   print("The calibrated difc at detid {detid} is {difc}".format(**mtd['cal'].row(40000)))

Output:

.. code-block:: none

  The calibrated difc at detid 40896 is 5523.060327692842

.. categories::

.. sourcelink::
