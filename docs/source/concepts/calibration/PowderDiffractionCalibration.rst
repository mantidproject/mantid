.. _Powder Diffraction Calibration:

Time-of-Flight Powder Diffraction Calibration
=============================================

.. contents::
  :local:


Data Required
-------------

To get a good calibration you will want good statistics with
calibration data. For most of the calibration algorithms, this means
having enough statistics in a single pixel to fit individual peaks or
to cross-correlate data. Some of the calibration algorithms also
require knowing the ideal positions of peaks in d-spacing. For these
algorithms, it is not uncommen to calibrate the instrument, refine the
results using a Rietveld program, then using the updated peak
positions to calibrate again. Often the sample selected will be diamond.

Calculating Calibration
-----------------------

Relative to a specific spectrum
###############################

This technique of calibration uses a reference spectrum to calibrate
the rest of the instrument to. The main algorithm that does this is
:ref:`GetDetectorOffsets <algm-GetDetectorOffsets>` whose
``InputWorkspace`` is the ``OutputWorkspace`` of :ref:`CrossCorrelate
<algm-CrossCorrelate>`. Generically the workflow is

1. :ref:`Load <algm-Load>` the calibration data
2. Convert the X-Units to d-spacing using :ref:`ConvertUnits
   <algm-ConvertUnits>`. The "offsets" calculated are relative
   reference spectrum's geometry so using :ref:`AlignDetectors
   <algm-AlignDetectors>` will violate the assumptions for other
   algorithms used with time-of-flight powder diffraction and give the
   wrong results for focused data.
3. Run :ref:`Rebin <algm-Rebin>` to set a common d-spacing bin
   structure across all of the spectra, you will need fine enough bins
   to allow fitting of your peak.  Whatever you choose, make a note of
   it you may need it later.
4. Take a look at the data using the SpectrumViewer or a color map
   plot, find the workspace index of a spectrum that has the peak
   close to the known reference position.  This will be the reference
   spectra and will be used in the next step.
5. Cross correlate the spectrum with :ref:`CrossCorrelate
   <algm-CrossCorrelate>`, enter the workspace index as the
   ``ReferenceSpectra`` you found in the last step.
6. Run :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>`, the
   InputWorkspace if the output from :ref:`CrossCorrelate
   <algm-CrossCorrelate>`.  Use the rebinning step you made a note of
   in step 3 as the step parameter, and DReference as the expected
   value of the reference peak that you are fitting to.  XMax and XMin
   define the window around the reference peak to search for the peak
   in each spectra, if you find that some spectra do not find the peak
   try increasing those values.
7. The output is an ``OffsetsWorspace``. See below for how to save,
   load, and use the workspace.

Using known peak positions
##########################

These techniques require knowing the precise location, in d-space, of
diffraction peaks and benefit from knowing
more. :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`
and :ref:`PDCalibration <algm-PDCalibration>` are the main choices for
this. Both algorithms fit individual peak positions and use those fits
to generate the calibration information.

The workflow for :ref:`GetDetOffsetsMultiPeaks
<algm-GetDetOffsetsMultiPeaks>` is identical to that of
:ref:`GetDetectorOffsets <algm-GetDetectorOffsets>` without the
cross-correlation step (5). The main difference in the operation of
the algorithm is that it essentially calculates an offset from each
peak then calculates a weighted average of those offsets for the
individual spectrum.

The workflow for :ref:`PDCalibration <algm-PDCalibration>` differs
significantly from that of the other calibration techniques. It
requires the data to be in time-of-flight, then uses either the
instrument geometry, or a previous calibration, to convert the peak
positions to time-of-flight. The individual peaks fits are then used
to calculate :math:`DIFC` values directly. The benefit of this method, is
that it allows for calibrating starting from a "good" calibration,
rather than returning back to the instrument geometry. The steps for
using this are

1. :ref:`Load <algm-Load>` the calibration data
2. Run :ref:`PDCalibration <algm-PDCalibration>` with appropriate
   properties
3. The ``OutputCalibrationTable`` is a :ref:`TableWorkspace <Table Workspaces>`. See
   below for how to save, load, and use the workspace.


Workflow algorithms
###################

:ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>`
will do most of the workflow for you, including applying the
calibration to the data. While its name suggests it is only for a
particular subset of detector types, it is not. It has many options
for selecting between :ref:`GetDetectorOffsets
<algm-GetDetectorOffsets>` and :ref:`GetDetOffsetsMultiPeaks
<algm-GetDetOffsetsMultiPeaks>`.

Saving and Loading Calibration
##############################

There are two basic formats for the calibration information. The
legacy ascii format is described in :ref:`CalFile`. The newer HDF5
version is described alongside the description of :ref:`calibration
table <DiffractionCalibrationWorkspace>`.

Saving and loading the HDF5 format is done with :ref:`SaveDiffCal
<algm-SaveDiffCal>` and :ref:`LoadDiffCal <algm-LoadDiffCal>`.

Saving and loading the legacy format is done with :ref:`SaveCalFile
<algm-SaveCalFile>` and :ref:`LoadCalFile <algm-LoadCalFile>`. This
can be converted from an ``OffsetsWorkspace`` to a calibration table
using :ref:`ConvertDiffCal <algm-ConvertDiffCal>`.

.. figure:: /images/PG3_Calibrate.png
  :width: 400px
  :align: right

Testing your Calibration
------------------------

.. figure:: /images/SNAP_Calibrate.png
  :width: 400px
  :align: right

The first thing that should be done is to convert the calibration
workspace (either table or ``OffsetsWorkspace`` to a workspace of
:math:`DIFC` values to inspect using the :py:obj:`instrument view
<mantidplot.InstrumentView>`. This can be done using
:ref:`CalculateDIFC <algm-CalculateDIFC>`. The values of :math:`DIFC`
should vary continuously across the detectors that are close to each
other (e.g. neighboring pixels in an LPSD).

You will need to test that the calibration managed to find a
reasonable calibration constant for each of the spectra in your data.
The easiest way to do this is to apply the calibration to your
calibration data and check that the bragg peaks align as expected.

1. Load the calibration data using :ref:`Load <algm-Load>`
2. Run :ref:`AlignDetectors <algm-AlignDetectors>`, this will convert the data to d-spacing and apply the calibration.  You can provide the calibration using the ``CalibrationFile``, the ``CalibrationWorkspace``, or ``OffsetsWorkspace``.
3. Plot the workspace as a Color Fill plot, in the spectrum view, or a few spectra in a line plot.

Further insight can be gained by comparing the grouped (after aligning
and focussing the data) spectra from a previous calibration or convert
units to the newly calibrated version. This can be done using
:ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` with and without
calibration information. In the end, a Rietveld refinement is the best
test of the calibration.

Expanding on detector masking
-----------------------------

While many of the calibration methods will generate a mask based on the detectors calibrated, sometimes additional metrics for masking are desired. One way is to use :ref:`DetectorDiagnostic <algm-DetectorDiagnostic>`. The result can be combined with an existing mask using

.. code::

   BinaryOperateMasks(InputWorkspace1='mask_from_cal', InputWorkspace2='mask_detdiag',
                      OperationType='OR', OutputWorkspace='mask_final')

Creating detector grouping
--------------------------

To create a grouping workspace for :ref:`SaveDiffCal
<algm-SaveDiffCal>` you need to specify which detector pixels to
combine to make an output spectrum. This is done using
:ref:`CreateGroupingWorkspace <algm-CreateGroupingWorkspace>`. An
alternative is to generate a grouping file to load with
:ref:`LoadDetectorsGroupingFile <algm-LoadDetectorsGroupingFile>`.


Adjusting the Instrument Definition
-----------------------------------

This approach attempts to correct the instrument component positions based on the calibration data. It can be more involved than applying the correction during focussing.

1. Perform a calibration using :ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>` or :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`.  Only these algorithms can export the :ref:`Diffraction Calibration Workspace <DiffractionCalibrationWorkspace>` required.
2. Run :ref:`AlignComponents <algm-AlignComponents>` this will move aspects of the instrument to optimize the offsets.  It can move any named aspect of the instrument including the sample and source positions.  You will likely need to run this several times, perhaps focussing on a single bank at a time, and then the source and sample positions in order to  get a good alignment.
3. Then either:

   * :ref:`ExportGeometry <algm-ExportGeometry>` will export the resulting geometry into a format that can be used to create a new XML instrument definition.  The Mantid team at ORNL have tools to automate this for some instruments at the SNS.
   * At ISIS enter the resulting workspace as the calibration workspace into the DAE software when recording new runs.  The calibrated workspace will be copied into the resulting NeXuS file of the run.



.. categories:: Calibration
