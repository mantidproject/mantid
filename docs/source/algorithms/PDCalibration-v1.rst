
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes an ``InputWorkspace`` containing powder diffraction spectra from a standard sample with x-units of time-of-flight (TOF).
These spectra are fitted to extract the Bragg peak positions. The extracted peak positions are then used to
determine the diffractometer constants that convert diffraction spectra from TOF to d-spacing. These constants are described in detail
in :ref:`algm-AlignDetectors`. The number of the constants being determined (1,2,or 3) is controlled by ``CalibrationParameters``.
The results are output to a :ref:`diffraction calibration workspace <DiffractionCalibrationWorkspace>`.

Unlike other calibration algorithms (see `Time-of-Flight Powder Diffraction Calibration <../concepts/calibration/PowderDiffractionCalibration.html>`_),
peak fitting in `PDCalibration` is done in TOF, not d-spacing. Correspondingly, the input ``PeakPositions`` and ``PeakWindow`` values are converted
from d-spacing to TOF. The conversion is based on either the old calibration (see ``PreviousCalibrationFile`` and ``PreviousCalibrationTable``) or,
if not provided, on the instrument geometry contained in the ``InputWorkspace``.

The peak fitting properties are explained in more detail in
:ref:`algm-FitPeaks`. This is used to perform a least-squares fitting of peaks
using as much information as is provided as possible. Each input
spectrum is calibrated separately following the same basic steps:

1. The ``PeakPositions`` are used to determine fit windows in combination with ``PeakWindow``. The windows are half the distance between the provided peak positions, with a maximum size of ``PeakWindow``.
2. The positions and windows are converted to TOF for the spectrum using either the previous calibration information or the instrument geometry.
3. For each peak, the background is estimated from the first and last ten points in the fit window.
4. For each peak, the nominal center is selected by locating the highest point near the expected position. The height is used as the initial guess as well.
5. For each peak, the width is estimated by multiplying the peak center position with ``PeakWidthPercent`` or by calculating FWHM of the data in the window.
6. For each peak, parameters (such as center, height, and width) are fitted with least-squares.
7. All of the fitted peak centers, weighted according to ``UseChiSq``, are used in a separate least-squares procedure to determine the diffractometer constants.

If more than one constant is requested, the result that has the lowest
`reduced chi-squared value
<https://en.wikipedia.org/wiki/Reduced_chi-squared_statistic>`_ is
returned. This favors using less parameters.

When not specified using the ``MaskWorkspace`` parameter, the default name for the mask workspace will be the ``OutputCalibrationTable`` parameter + ``_mask``.  If the mask workspace already exists, its masked values will be combined with those from any uncalibrated pixels detected during the algorithm's execution.

The resulting calibration table can be saved with
:ref:`algm-SaveDiffCal`, loaded with :ref:`algm-LoadDiffCal` and
applied to a workspace with :ref:`algm-AlignDetectors`. There are also
three workspaces placed in the ``DiagnosticWorkspace`` group. They are:

* evaluated fit functions (``_fitted``) which is the ``OutputPeakParametersWorkspace`` from :ref:`FitPeaks <algm-FitPeaks>`
* raw peak fit values (``_fitparam``) which is the ``FittedPeaksWorkspace`` from :ref:`FitPeaks <algm-FitPeaks>`
* uncertainties in raw fit values (``_fiterror``) which is the ``OutputParameterFitErrorsWorkspace`` from :ref:`FitPeaks <algm-FitPeaks>` when ``UseChisSq=True`` is set
* peak fitted positions in d-space ( ``_dspacing``) derived from the effective peak parameters
* peak widths (``_width``) derived from the effective peak parameters
* peak heights (``_height``) derived from the effective peak parameters
* instrument resolution (delta-d/d ``_resolution``) derived from the average of effective width/height of each peak.
  This is only correct for Gaussian and Lorentzian peak shapes

Since multiple peak shapes can be used,
see the documentation for the individual :ref:`fit functions
<Fit Functions List>` to see how they relate to the effective
values displayed in the diagnostic tables. For ``Gaussian`` and
``Lorentzian``, the widths and resolution are converted to values that
can be directly compared with the results of
:ref:`algm-EstimateResolutionDiffraction`.

Limiting Spectra Calibrated
---------------------------

Supplying ``StartWorkspaceIndex`` and/or ``StopWorkspaceIndex`` will limit the spectra that are fitted.
Only those that are fitted will exist in the output table, ``OutputCalibrationTable``.
:ref:`CombineDiffCal <algm-CombineDiffCal>` can accept input of partial instrument calibration as the ``GroupedCalibration`` and will copy all other values fom the ``PixelCalibration``.
In this mode, the ``CalibrationWorkspace`` supplied to :ref:`CombineDiffCal <algm-CombineDiffCal>`  should still be the ``InputWorkspace`` supplied to ``PDCalibration``.

Usage
-----

**Example - PDCalibration**

.. code-block:: python

   # If you have an old calibration it can be used as the starting point
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

**Example - PDCalibration with BackToBackExponential fit function**

The following example shows how to use PDCalibration with the BackToBackExponential fit function. The fit works best if sensible initial values for the parameters are specified in an instrument definition or parameter file (for more details, see the :ref:`fitting parameters <Using fitting parameter>` documentation):

.. code-block:: python

   Load(Filename=r'ENGINX00193749.nxs', OutputWorkspace='193749')
   dpks = (1.913220892, 1.631600313,
           1.562138267, 1.352851554, 1.104598643)

   # initial values for GSAS parameters A, B, S are in ENGINX parameters .xml
   # use log binning
   PDCalibration(InputWorkspace='193749',
                 TofBinning=[10000,-0.0005,46000],
                 PeakPositions=dpks,
                 PeakWindow = 0.03,
                 MinimumPeakHeight = 0.5,
                 PeakFunction = 'BackToBackExponential',
                 CalibrationParameters = 'DIFC',
                 OutputCalibrationTable='cal_B2B_DIFC_chisqTrue',
                 DiagnosticWorkspaces = 'diag_B2B_DIFC_chisqTrue',
                 UseChiSq = True)

   # Print the result
   print("The calibrated difc at detid {detid} is {difc}".format(**mtd['cal_B2B_DIFC_chisqTrue'].row(1000)))

Output:

.. code-block:: none

  The calibrated difc at detid 108041 is 16834.952770921267

.. categories::

.. sourcelink::
