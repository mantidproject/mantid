.. _Powder Diffraction Calibration:

Powder Diffraction Calibration
==============================
  
.. contents::
  :local:


Calculating Calibration
-----------------------

Data Required
#############

You will need to run a sample which will create good clean peaks at known reference dSpacing positions.  To get a good calibration you will want good statistics on this calibration data.

Steps for rectangular detector based instruments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you are trying to calibrate an instrument that uses rectangular detectors the SNS  have created a single algorithm to do all of this for you, :ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>`.  At the time of writing this only supports SNS instruments, but look at the algorithm link for an up to date list.

Steps for other instruments
^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Load the calibration data using :ref:`Load <algm-Load>` 
2. Convert the X-Units to d-spacing using :ref:`ConvertUnits <algm-ConvertUnits>`
3. Run :ref:`Rebin <algm-Rebin>` to set a common d-spacing bin structure across all of the spectra, you will need fine enough bins to allow fitting of your peak.  Perhaps 0.001 as a rebin step, whatever you choose, make a note of it  you may need it later.
4. Take a look at the data using the SpectrumVeiwer or a color map plot, find the workspace index of a spectrum that has the peak close to the known reference position.  This will be the reference spectra and will be used in the next step.
5. Here the steps can split depending on whether you are calibrating on a single peak, or a range of peaks


   * **Single Peak calibration** (this requires the same peak in all detectors)

     6. Cross correlate the spectrum with :ref:`CrossCorrelate <algm-CrossCorrelate>`, enter the workspace index as the ReferenceSpectra you found in the last step.
     7. Run :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>`, the InputWorkspace if the output from :ref:`CrossCorrelate <algm-CrossCorrelate>`.  Use the rebinning step you made a note of in step 3 as the step parameter, and DReference as the expected value of the reference peak that you are fitting to.  XMax and XMin define the window around the reference peak to search for the peak in each spectra, if you find that some spectra do not find the peak try increasing those values.
     8. The output is an OffsetsWorspace, that can be used directly in :ref:`DiffractionFocussing <algm-DiffractionFocussing>`, or saved using :ref:`SaveCalFile <algm-SaveCalFile>`.  You can also save it as a :ref:`CalFile` from :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>`, by defining the GroupingFileName parameter.
       
   * **Multi Peak Calibration** (*This is less well tested*)
     If you do not have a single peak in all detectors, but a range of known peaks across detectors you can try this approach. Another possible approach is to perform the single peak calibration across sections of the instrument with different reference peaks and combine the output calibration.

     6. Run :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`, the Input workspace is the one from step 3 earlier.  For DReference you can enter a comma seperated list of the d-spacing values of the known peaks.
     7. The output is an OffsetsWorspace, and a workspace with the number of peaks found in each spectra,  The output offsets workspace that can be used directly in :ref:`DiffractionFocussing <algm-DiffractionFocussing>`, or saved using :ref:`SaveCalFile <algm-SaveCalFile>`.  You can also save it as a :ref:`CalFile` from :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`, by defining the GroupingFileName parameter.


Additionally :ref:`PDCalibration <algm-PDCalibration>` can be used to fit peaks directly in TOF instead of converting to d-spacing. This algorithm produces a calibration table which can be passed to :ref:`AlignDetectors <algm-AlignDetectors>`. The algorithms :ref:`LoadDiffCal <algm-LoadDiffCal>` and :ref:`SaveDiffCal <algm-SaveDiffCal>` can be used to read and write the calibration table to file.
     
.. figure:: /images/PG3_Calibrate.png
  :width: 400px
  :align: right

Testing your Calibration
------------------------

.. figure:: /images/SNAP_Calibrate.png
  :width: 400px
  :align: right

You will need to test that the calibration managed to find a reasonable offset for each of the spectra in your data.
The easiest way to do this is to apply the calibration to your calibration data and check that the bragg peaks align as expected.

1. Load the calibration data using :ref:`Load <algm-Load>` 
2. Run :ref:`AlignDetectors <algm-AlignDetectors>`, this will convert the data to d-spacing and apply the calibration.  You can provide the calibration either by defining the OffsetsWrokspace, or by providing the path to the saved :ref:`CalFile`.
3. Plot the workspace as a Color Fill plot, or a few spectra as a line plot.

Applying your calibration
-------------------------

During Focussing
################

The calibration can be applied as part of the reduction and processing workflow using the two algorithms 

1. Load the experimental data using :ref:`Load <algm-Load>` 
2. Run :ref:`AlignDetectors <algm-AlignDetectors>`, this will convert the data to d-spacing and apply the calibration.  You can provide the calibration either by defining the OffsetsWrokspace, or by providing the path to the saved :ref:`CalFile`.
3. Run :ref:`DiffractionFocussing <algm-DiffractionFocussing>` with the output from AlignDetectors as the input.  This will group the detectors according to the GroupingWorkspace or CalFile.

Adjusting the Instrument Definition
###################################

This approach attempts to correct the instrument component positions based on the calibration data. It can be more involved than applying the correction during focussing.

1. Perform a calibration using :ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>` or :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`.  Only these algorithms can export the :ref:`Diffraction Calibration Workspace <DiffractionCalibrationWorkspace>` required.
2. Run :ref:`AlignComponents <algm-AlignComponents>` this will move aspects of the instrument to optimize the offsets.  It can move any named aspect of the instrument including the sample and source positions.  You will likely need to run this several times, perhaps focussing on a single bank at a time, and then the source and sample positions in order to  get a good alignment.
3. Then either:
   * :ref:`ExportGeometry <algm-ExportGeometry>` will export the resulting geometry into a format that can be used to create a new XML instrument definition.  The Mantid team at ORNL have tools to automate this for common instruments at the SNS.
   * At ISIS enter the resulting workspace as the calibration workspace into the DAE software when recording new runs.  The calibrated workspace will be copied into the resulting NeXuS file of the run.
  


.. categories:: Calibration
