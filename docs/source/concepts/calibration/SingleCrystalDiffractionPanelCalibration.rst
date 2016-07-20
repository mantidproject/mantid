.. _Single Crystal Diffraction Panel Calibration:

Single Crystal Diffraction Panel Calibration
============================================

.. contents::
  :local:


Calculating Calibration
-----------------------

Notes
#####

* This approach has been imported from ISAW (SCD Calibration)
* Only works for :ref:`RectangularDetector`

Data Required
#############

You will need :ref:`PeaksWorkspace` with indexed peaks from multiple orientations.

Steps for rectangular detector based instruments
################################################

:ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` is the prime algorithm that performs this calibration.  Take a look the  description and workflow sections of the :ref:`algorithm documentation  <algm-SCDCalibratePanels>` for more details. If you set a value for DetCalFilename then it will output a DetCal file for :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>`.


Applying your calibration
#########################

After calibration, you can save the workspace to Nexus (or Nexus processed) and get it back by loading in a later Mantid session. You can copy the calibration to another workspace using the same instrument by means of the :ref:`CopyInstrumentParameters <algm-CopyInstrumentParameters>` algorithm. To do so select the workspace, which you have calibrated as the InputWorkspace and the workspace you want to copy the calibration to, the OutputWorkspace.

.. categories:: Calibration
