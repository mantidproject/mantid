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

You will need to run a sample which will create good clean peaks at known reference dSpacing positions.  To get a good calibration you will want good statistics on this calibration data.

Using the Engineering GUI
#########################

*  Custom GUI: [calibration tab](http://www.mantidproject.org/File:Engggui_36_calib_tab.png)

Calibrating the entire instrument
#################################

* Calibration of every detector/pixel: :ref:`EnggCalibrateFull <algm-EnggCalibrateFull>` 

Calibrating individual banks
############################

* Calibration of banks:  :ref:`EnggCalibrate <algm-EnggCalibrate>`
  
Under the hood
##############

*  Under the hood: :ref:`EnggFitPeaks <algm-EnggFitPeaks>`, :ref:`FindPeaks <algm-FindPeaks>`,  :ref:`EnggVanadiumCorrections <algm-EnggVanadiumCorrections>`
*  Peak functions (shapes):  [Back2BackExponential](http://docs.mantidproject.org/nightly/fitfunctions/BackToBackExponential.html) (in EnggFitPeaks), [Gaussian](http://docs.mantidproject.org/nightly/fitfunctions/Gaussian.html) (in FindPeaks), Bk2BkExpConvPV (in GSAS).

File Formats
############

*  The legacy ascii/csv format that you should never use: [ENGINX_full_pixel_calibration_vana194547_ceria193749.csv](https://github.com/mantidproject/mantid/blob/master/scripts/Engineering/calib/ENGINX_full_pixel_calibration_vana194547_ceria193749.csv)
*  The [HDF format](http://docs.mantidproject.org/nightly/concepts/DiffractionCalibrationWorkspace.html) that should/will be used.

Applying your calibration
#########################



.. categories:: Calibration
