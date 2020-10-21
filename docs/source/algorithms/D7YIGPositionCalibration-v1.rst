
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that performs wavelength and position calibration for both individual detectors and detector banks using measurement of a sample of powdered :math:`\text{Y}_{3}\text{Fe}_{5}\text{O}_{12}` (YIG). This data is fitted with Gaussian distributions at the expected peak positions. The output is an :ref:`Instrument Parameter File <InstrumentParameterFile>` readable by the :ref:`LoadILLPolarizedDiffraction <algm-LoadILLPolarizedDiffraction>` algorithm that will place the detector banks and detectors using the output of this algorithm.

It is crucial for a reliable calibration to first run a test without setting the BankOffsets parameter and checking the positions of the YIG Bragg peaks in the `conjoined_input` workspace. Then, the BankOffsets can be obtained by comparing the peak positions coming from the detectors with their expected location, coming from known d-spacings for YIG.

Currently the algorithm is focused on the D7 instrument that has three detector banks and a monitor, and all of them can move individually. The detector position and wavelength calibration have to be checked each time the used wavelength is changed, since the instrument has to be manually moved into a different parking position around the monochromator.

Calibration method
##################

A YIG scan data is loaded into a single 2D Workspace where the X axis contains detector positions during each step of the scan, and the Y axis the measured intensities normalized to monitor. The Y axis data is also 2D with each row representing full scan for a single detector. Scan points between -35 and 15 degrees are masked to remove the impact of the direct neutron beam.

The provided YIG d-spacing values are loaded from an XML list. The default d-spacing is coming from Ref. [2]. The peak positions are converted into 2theta positions using the initial assumption of the neutron wavelength. All peaks that would require the 2theta to be above 180 degrees are removed.

YIG peaks in the detector's scan are fitted separately using a Gaussian distribution. The fitting results are stored in a new workspace, where the Y axis contains the fitted peak centres and the X axis the calculated peak positions.


The workspace containing the peak fitting results is then fitted using a `Multidomain` function of the form:

.. math:: 2\theta_{fit} = m \cdot (2.0 \cdot \text{asin} ( \lambda / 2d ) + offset_{\text{pixel}} + offset_{\text{bank}}),

where `m` is the bank slope, :math:`offset_{\text{pixel}}` is the relative offset to the initial assumption of the position inside the detector bank, and :math:`offset_{\text{bank}}` is the offset of the entire bank. This function allows to extract the information about the wavelength, detector bank slopes and offsets, and the distribution of detector offsets.


Usage
-----
.. include:: ../usagedata-note.txt

**Example - D7YIGPositionCalibration - calibration at the shortest wavelength**

.. code-block:: python

   approximate_wavelength = '3.1' # Angstrom
   D7YIGPositionCalibration(Filenames='402652:403041', ApproximateWavelength=approximate_wavelength,
                               YIGPeaksFile='D7_YIG_peaks.xml', CalibrationOutputFile='test_shortWavelength.xml',
                               MinimalDistanceBetweenPeaks=1.5, BankOffsets="-3,-3,1", ClearCache=True,
                               FitOutputWorkspace='shortWavelength')
		       
   print('The calibrated wavelength is: {0:.2f}'.format(float(approximate_wavelength)*mtd['shortWavelength'].column(1)[1]))
   print('The bank2 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[0]))
   print('The bank3 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[176]))
   print('The bank4 gradient is: {0:.3f}'.format(1.0 / mtd['shortWavelength'].column(1)[352]))

**Example - D7YIGPositionCalibration - calibration at the intermediate wavelength**

.. testcode:: D7YIGCalibrationIntermediateExample

   Load('ILL/D7/396442_396831.nxs', OutputWorkspace='intermediateWavelengthScan')
   approximate_wavelength = '4.8' # Angstrom
   D7YIGPositionCalibration(InputWorkspace='intermediateWavelengthScan', ApproximateWavelength=approximate_wavelength,
                               YIGPeaksFile='D7_YIG_peaks.xml', CalibrationOutputFile='test_intermediateWavelength.xml',
                               MinimalDistanceBetweenPeaks=1.5, BankOffsets="-4,-4,0",
                               FitOutputWorkspace='intermediateWavelength')
		       
   print('The calibrated wavelength is: {0:.2f}'.format(float(approximate_wavelength)*mtd['intermediateWavelength'].column(1)[1]))
   print('The bank2 gradient is: {0:.2f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[0]))
   print('The bank3 gradient is: {0:.2f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[176]))
   print('The bank4 gradient is: {0:.2f}'.format(1.0 / mtd['intermediateWavelength'].column(1)[352]))

Output:

.. testoutput:: D7YIGCalibrationIntermediateExample

   The calibrated wavelength is: 4.86
   The bank2 gradient is: 0.99
   The bank3 gradient is: 1.00
   The bank4 gradient is: 1.00

#. T. Fennell, L. Mangin-Thro, H.Mutka, G.J. Nilsen, A.R. Wildes.
   *Wavevector and energy resolution of the polarized diffuse scattering spectrometer D7*,
   Nuclear Instruments and Methods in Physics Research A **857** (2017) 24–30
   `doi: 10.1016/j.nima.2017.03.024 <https://doi.org/10.1016/j.nima.2017.03.024>`_

#. A. Nakatsuka, A. Yoshiasa, and S. Takeno.
   *Site preference of cations and structural variation in Y3Fe5O12 solid solutions with garnet structure
   Acta Crystallographica Section B **51** (1995) 737–745
   `doi: 10.1107/S0108768194014813 <https://doi.org/10.1107/S0108768194014813>`_


.. categories::

.. sourcelink::
