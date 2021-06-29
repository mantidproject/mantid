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
- :ref:`ConvertDiffCal <algm-ConvertDiffCal-v1>` now optionally updates a previous calibration when converting offsets.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` major interface update along with enabling the calibration of T0 and sample position.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` minor interface update that allows fine control of bank rotation calibration.
- :ref:`PDCalibration <algm-PDCalibration-v1>` has a new option to use the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak function.
- :ref:`SNAPReduce <algm-SNAPReduce-v1>` permits saving selected property names and values to file, to aid autoreduction.
- Add a custom ttmode to the PEARL powder diffraction scripts for running with a custom grouping file
- improve performance of :ref:`ApplyDiffCal <algm-ApplyDiffCal>` on large instruments eg WISH. This in turn improves the performance of :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`

Bugfixes
########
- Fix the issue with :ref:`SNSPowderReduction <algm-SNSPowderReduction>` - when invalid height unit is encountered while reading sample log, we should continue by ignoring geometry and rely purely on user input.
- fix d-spacing calculation when parabolic model is selected.

Engineering Diffraction
-----------------------
New features
############
- Added serial fit capability to fitting tab in EngDiff UI - this fits all loaded workspaces with same initial parameters.

Improvements
############
- The workflows for Calibration and Focusing in the EnggDiffraction GUI and EnginX scripts have been replaced to make use of faster, better tested C++ algorithms (PDCalibration) - as a result the following algorithms have been deprecated, and will likely be removed entirely in the next release: EnggCalibrate, EnggCalibrateFull, EnggFocus, EnggVanadiumCorrections.

Bugfixes
########
- Sequential fitting in the EngDiff UI now uses the output of the last successful fit (as opposed to the previous fit) as the initial parameters for the next fit.
- An empty Engineering Diffraction interface is no longer saved if the user saves a project having previously had the interface open at some point in that session
- The help button on the Engineering Diffraction interface points to the correct page, having been broken in the last release

Single Crystal Diffraction
--------------------------
New features
############
- New algorithm :ref:`HB3AIntegrateDetectorPeaks <algm-HB3AIntegrateDetectorPeaks>` for integrating four-circle data from HB3A in detector space.
- New algorithm :ref:`ApplyInstrumentToPeaks <algm-ApplyInstrumentToPeaks>` to update the instrument of peaks within a PeaksWorkspace.
- New plotting script that provides diagnostic plots of SCDCalibratePanels output.
- New plotting script that provides diagnositc plots of SCDCalibratePanels2 on a per panel/bank basis.
- New algorithm :ref:`ConvertPeaksWorkspace <algm-ConvertPeaksWorkspace>` for quick conversion between PeaksWorkspace and LeanElasticPeaksWorkspace.

Improvements
############
- Find detector in peaks will check which det is closer when dealing with peak-in-gap situation for tube-type detectors.
- Existing :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now provides better calibration of panel orientation for flat panel detectors.
- Existing :ref:`MaskPeaksWorkspace <algm-MaskPeaksWorkspace-v1>` now also supports tube-type detectors used at the CORELLI instrument.
- Existing :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` now retains the value of small optimization results instead of zeroing them.

Bugfixes
########
- Expand the Q space search radius in DetectorSearcher to avoid missing peaks when using :ref:`PredictPeaks <algm-PredictPeaks>`.

:ref:`Release 6.2.0 <v6.2.0>`
