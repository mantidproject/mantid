
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
geometry. The SignalFile or SignalWorkspace contains the data from a
standard sample. The results are then fitted with up to difc, t_zero
and difa, these values are details in :ref:`algm-AlignDetectors`.

The peak fitting properties are explained in
:ref:`algm-FindPeaks`. This also uses the same criteria on peaks as
:ref:`algm-GetDetOffsetsMultiPeaks`.

A mask workspace is created, named "OutputCalibrationTable" + '_mask',
with uncalibrated pixels masked.

The resulting calibration table can be saved with
:ref:`algm-SaveDiffCal`, loaded with :ref:`algm-LoadDiffCal` and
applied to a workspace with :ref:`algm-AlignDetectors`. There are also
three workspaces placed in the ``DiagnosticWorkspace`` group. They
contain the fitted positions in dspace( ``_dspacing``), peak widths
(``_width``), peak heights (``_height``), raw peak fit values (``_fitparam``),
evaluated fit functions (``_fitparam``), and instrument resolution
(delta-d/d ``_resolution``). Since multiple peak shapes can be used,
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
                 PreviousCalibrationTable=oldCal,
                 PeakPositions=dvalues,
                 OutputCalibrationTable='cal',
                 DiagnosticWorkspaces='diag')

   # Print the result
   print("The calibrated difc at detid {detid} is {difc}".format(**mtd['cal'].row(40000)))

Output:

.. code-block:: none

  The calibrated difc at detid 40896 is 5523.060327692842

.. categories::

.. sourcelink::
