
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the algorithm that performs wavelength and position calibration for both individual detectors and detector banks using the YIG scan data. The output is an Instrument Parameter File readable by the LoadILLPolarizedDiffraction algorithm that will place the detector banks and detectors using the output of this algorithm.

It is crucial for a reliable calibration to first run a test without setting the BankOffsets parameter and checking the positions of the YIG Bragg peaks in the `intensityWS` workspace. Then, the BankOffsets can be obtained by comparing the peak positions coming from the detectors with their expected location, coming from known d-spacings for YIG.

Currently the algorithm is focused on the D7 instrument.

Calibration method
##################

A YIG scan data is loaded into a single 2D Workspace where the X axis contains detector positions during each step of the scan, and the Y axis the measured intensities normalized to monitor. The Y axis data is also 2D with each row representing full scan for a single detector. Scan points between -35 and 15 degrees are masked to remove the impact of the direct neutron beam.

The provided YIG d-spacing values are loaded from an XML list. The peak positions are converted into 2theta positions using the initial assumption of the neutron wavelength. All peaks that would require the 2theta to be above 180 degrees are removed.

YIG peaks in the detector's scan are fitted separately using a Gaussian distribution. The fitting results are stored in a new workspace, where the Y axis contains the fitted peak centres and the X axis the calculated peak positions.

The workspace containing the peak fitting results is then fitted using a `Multidomain` function of the form: `2theta_{fit} = m * (2.0 * asin(wavelength / 2d) + pixel_offset + bank_offset)`, where m is the bank slope, pixel_offset is the relative offset to the initial assumption of the position inside the detector bank, and bank_offset is the offset of the entire bank. This function allows to extract the information about the wavelength, detector bank slopes and offsets, and the distribution of detector offsets.


Usage
-----
.. include:: ../usagedata-note.txt

**Example - ILLD7YIGCalibration - calibration at the intermediate wavelength**

.. testcode:: ILLD7YIGCalibrationIntermediateExample

   Load('ILL/D7/396442_396831.nxs', OutputWorkspace='intermediateWavelengthScan')
   approximate_wavelength = '4.8' # Angstrom
   ILLD7YIGPositionCalibration(ScanWorkspace='intermediateWavelengthScan', ApproximateWavelength=approximate_wavelength,
                               YIGPeaksFile='ILL/D7/YIG_peaks.xml', CalibrationFilename='test_intermediateWavelength.xml',
                               MinimalDistanceBetweenPeaks=1.5, BankOffsets="-4,-4,0",
                               DetectorFitOutput='intermediateWavelength')
		       
   print('The calibrated wavelength is: {0:.2f}'.format(float(approximate_wavelength)*mtd['intermediateWavelength'].column(1)[1]))
   print('The bank2 gradient is: {0:.3f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[0]))
   print('The bank3 gradient is: {0:.3f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[176]))
   print('The bank4 gradient is: {0:.3f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[352]))

Output:

.. testoutput:: ILLD7YIGCalibrationIntermediateExample

   The output calibrated wavelength is: 4.86
   The bank2 gradient is: 0.992
   The bank3 gradient is: 1.004
   The bank4 gradient is: 0.997

.. categories::

.. sourcelink::
