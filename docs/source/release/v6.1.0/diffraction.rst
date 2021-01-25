===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------
New features
############

- New algorithm :ref:`RebinRagged <algm-RebinRagged>` which can rebin a workspace with different binning parameters for each spectrum
- :ref:`PDCalibration <algm-PDCalibration>` now supports workspaces with grouped detectors (i.e. more than one detector per spectrum).

Improvements
############

- :ref:`PDCalibration <algm-PDCalibration>` now intitialises A,B and S of BackToBackExponential if correpsonding coeficients are in the instrument parameter.xml file.

Bugfixes
########

- Fix out-of-range bug in :ref:`FitPeaks <algm-FitPeaks>` for histogram data.
- Fix bug to actually implement intended sequential fit of DIFC, DIFA, TZERO in :ref:`PDCalibration <algm-PDCalibration>`.


Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------
- New version of algorithm :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` provides more accurate calibration results for CORELLI instrument.

:ref:`Release 6.1.0 <v6.1.0>`
