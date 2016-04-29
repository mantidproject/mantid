.. _Calibration:

Calibration
===========

.. contents::
  :local:

What is Calibration?
--------------------

Calibration is a process which determines and corrects for differences in the definition of an instrument to reality, generally conducted by peorforming a measurement of a reference compound under known conditions.



Supported Types of Calibration
------------------------------

* :ref:`Powder Diffraction Calibration`
* :ref:`Single Crystal Diffraction Panel Calibration`
* :ref:`Engineering Calibration`
* :ref:`PSD Tube Calibration`

Single Crystal Diffraction Panels
#################################

  * :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>`
  
    * Copied from ISAW (SCD Calibration), but ISAW's calibration is still better
    * Only works for RectangularDetectors
    * Input is PeaksWorkspace with peaks from multiple orientations
    * Output is DetCal file for LoadIsawDetCal
  
Engineering
###########

  * Calibration of every detector/pixel: :ref:`EnggCalibrateFull <algm-EnggCalibrateFull>` 
  * Calibration of banks:  :ref:`EnggCalibrate <algm-EnggCalibrate>` 
  *  The legacy ascii/csv format that you should never use: [ENGINX_full_pixel_calibration_vana194547_ceria193749.csv](https://github.com/mantidproject/mantid/blob/master/scripts/Engineering/calib/ENGINX_full_pixel_calibration_vana194547_ceria193749.csv)
  *  The [HDF format](http://docs.mantidproject.org/nightly/concepts/DiffractionCalibrationWorkspace.html) that should/will be used.
  *  Under the hood: :ref:`EnggFitPeaks <algm-EnggFitPeaks>`, :ref:`FindPeaks <algm-FindPeaks>`,  :ref:`EnggVanadiumCorrections <algm-EnggVanadiumCorrections>`
  *  Custom GUI: [calibration tab](http://www.mantidproject.org/File:Engggui_36_calib_tab.png)
  *  Peak functions (shapes):  [Back2BackExponential](http://docs.mantidproject.org/nightly/fitfunctions/BackToBackExponential.html) (in EnggFitPeaks), [Gaussian](http://docs.mantidproject.org/nightly/fitfunctions/Gaussian.html) (in FindPeaks), Bk2BkExpConvPV (in GSAS).

.. categories:: Concepts Calibration
