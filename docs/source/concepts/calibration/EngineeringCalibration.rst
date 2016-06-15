.. _Engineering Calibration:

Engineering Calibration
=======================
  
.. contents::
  :local:


Calculating Calibration
-----------------------

Notes
#####

* The engineering Calibration is currently implemented to support Engin-X at ISIS, it may work for other instruments, but is subject to change at this point.

Data Required
#############

You will need to run a sample which will create good clean peaks at known reference dSpacing positions.  To get a good calibration you will want good statistics on this calibration data.  Vanadium and Ceria are common samples used for calibration.

.. interface:: Engineering Diffraction
  :align: right
  :width: 400

Using the Engineering GUI
#########################

Mantid provides a graphical user interface that  calculate can calibrations and
visualize them.

It is possible to load an existing calibration (as a CSV file) and to
generate a new calibration file (which becomes the new current
calibration).

A description of the inte3rface and all of it's controls can be found 
:ref:`here <ui engineering calibration>`.

Calibrating the entire instrument
#################################

Calibration of every detector/pixel at the same time is done by :ref:`EnggCalibrateFull <algm-EnggCalibrateFull>`.  This calculates the calibration in a single step and outputs a table workspace of the calibration.  As it is calibrating the whole instrument it can take some time, the next section shows how you can perform a partial calibration.

Calibrating individual banks
############################

:ref:`EnggCalibrate <algm-EnggCalibrate>` allows calibration of individual bank within the instrument, which is useful if only one bank has moved from a previous calibration, or if the calibration failed for a certain area of the instrument.  
  
Under the hood
##############

All of these approaches use the algorithm :ref:`EnggFitPeaks <algm-EnggFitPeaks>`, :ref:`FindPeaks <algm-FindPeaks>` and  :ref:`EnggVanadiumCorrections <algm-EnggVanadiumCorrections>` to find, and fit the recorded peaks and compare them to the expected values.  

The peak functions (shapes):  

* :ref:`BackToBackExponential<func-BackToBackExponential>` (used in EnggFitPeaks, to fit the peaks when accuracy is needed)
* :ref:`Gaussian<func-Gaussian>` (used in FindPeaks when the a quick validation is needed, but fit accuracy is not vital)

File Formats
############

*  The legacy ascii/csv format: [ENGINX_full_pixel_calibration_vana194547_ceria193749.csv](https://github.com/mantidproject/mantid/blob/master/scripts/Engineering/calib/ENGINX_full_pixel_calibration_vana194547_ceria193749.csv)
   This can get very large and cumbersome to deal with for instruments with many detectors.
*  The :ref:`HDF format<DiffractionCalibrationWorkspace>`.
   This is that is more compact and is increasingly being used.

Applying your calibration
#########################

The result of the calibration (the output table given in OutDetPosTable) is accepted by both :ref:`EnggCalibrate<algm-EnggCalibrate>` and :ref:`EnggFocus<algm-EnggFocus>` which use the columns ‘Detector ID’ and ‘Detector Position’ of the table to correct the detector positions before focusing.  The OutDetPosTable output table can also be used to apply the calibration calculated by this algorithm on any other workspace by using the algorithm :ref:`ApplyCalibration<algm-ApplyCalibration>`.

.. categories:: Calibration
