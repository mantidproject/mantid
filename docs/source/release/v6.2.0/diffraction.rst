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

Improvements
############
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` major interface update along with enabling the calibration of T0 and sample position.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` minor interface update that allows fine control of bank rotation calibration.

Bugfixes
########
- fix d-spacing calculation when parabolic model is selected.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------
New features
############
- New algorithm :ref:`HB3AIntegrateDetectorPeaks <algm-HB3AIntegrateDetectorPeaks>` for integrating four-circle data from HB3A in detector space.
- New algorithm :ref:`ApplyInstrumentToPeaks <algm-ApplyInstrumentToPeaks>` to update the instrument of peaks within a PeaksWorkspace.
- New plotting script that provides diagnostic plots of SCDCalibratePanels output.

Improvements
############
- Find detector in peaks will check which det is closer when dealing with peak-in-gap situation for tube-type detectors.
- Existing :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now provides better calibration of panel orientation for flat panel detectors.
- Existing :ref:`MaskPeaksWorkspace <algm-MaskPeaksWorkspace-v1>` now also supports tube-type detectors used at the CORELLI instrument.

Bugfixes
########
- Expand the Q space search radius in DetectorSearcher to avoid missing peaks when using :ref:`PredictPeaks <algm-PredictPeaks>`.

:ref:`Release 6.2.0 <v6.2.0>`
