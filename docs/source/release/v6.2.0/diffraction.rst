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
- New algorithm :ref:`CombineDiffCal <algm-CombineDiffCal>` to calibrate groups of pixels after cross correlation so that diffraction peaks can be adjusted to the correct positions
- New script for doing calibration by groups, :ref:`PowderDiffractionCalibration <calibration_tofpd_group_calibration-ref>`
- New algorithm :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` to compute the multiple scattering correction factor for sample using numerical integration.

Improvements
############
- :ref:`ConvertDiffCal <algm-ConvertDiffCal-v1>` now optionally updates a previous calibration when converting offsets.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` major interface update along with enabling the calibration of T0 and sample position.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` minor interface update that allows fine control of bank rotation calibration.
- :ref:`PDCalibration <algm-PDCalibration-v1>` has a new option to use the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak function.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder-v1>` permits masking of discrete wavelength ranges to zero, for resonance filtering
- :ref:`SNAPReduce <algm-SNAPReduce-v1>` permits saving selected property names and values to file, to aid autoreduction.
- :ref:`LoadWANDSCD <algm-LoadWANDSCD-v1>` now has a new option to perform normalization in the same loading process.

Bugfixes
########
- Fix the issue with :ref:`SNSPowderReduction <algm-SNSPowderReduction>` - when invalid height unit is encountered while reading sample log, we should continue by ignoring geometry and rely purely on user input.
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
- New plotting script that provides diagnositc plots of SCDCalibratePanels2 on a per panel/bank basis.
- Exposed :meth:`mantid.api.IPeak.getCol` and :meth:`mantid.api.IPeak.getRow` to python
- Added two integration methods to :ref:`HB3AIntegrateDetectorPeaks <algm-HB3AIntegrateDetectorPeaks>` for simple cuboid integration with and without fitted background.
- New algorithm :ref:`ConvertPeaksWorkspace <algm-ConvertPeaksWorkspace>` for quick conversion between PeaksWorkspace and LeanElasticPeaksWorkspace.

Improvements
############
- Find detector in peaks will check which det is closer when dealing with peak-in-gap situation for tube-type detectors.
- Existing :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now provides better calibration of panel orientation for flat panel detectors.
- Existing :ref:`DGSPlanner <dgsplanner-ref>` expanded to support WAND²
- Existing :ref:`MaskPeaksWorkspace <algm-MaskPeaksWorkspace-v1>` now also supports tube-type detectors used at the CORELLI instrument.
- Existing :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now retains the value of small optimization results instead of zeroing them.
- Existing :ref:`IntegrateEllipsoids <algm-IntegrateEllipsoids-v1>` now can use a different integrator for satellite peaks.

Bugfixes
########
- Expand the Q space search radius in DetectorSearcher to avoid missing peaks when using :ref:`PredictPeaks <algm-PredictPeaks>`.

:ref:`Release 6.2.0 <v6.2.0>`
