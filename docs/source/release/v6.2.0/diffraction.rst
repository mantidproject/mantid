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
- New algorithm :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>` inspects the sample enviroment logs for sample material and geometry information
- New script for doing calibration by groups, :ref:`PowderDiffractionCalibration <calibration_tofpd_group_calibration-ref>`
- New algorithm :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` to compute the multiple scattering correction factor for sample using numerical integration.

Improvements
############
- Documentation added for the group calibration routine for :ref:`PowderDiffractionCalibration <calibration_tofpd_group_calibration-ref>`, as as guidance for general users.
- The group calibration routine for :ref:`PowderDiffractionCalibration <calibration_tofpd_group_calibration-ref>` is made more generic. Groups are now allowed with dedicated control parameters.
- :ref:`ConvertDiffCal <algm-ConvertDiffCal-v1>` now optionally updates a previous calibration when converting offsets.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` major interface update along with enabling the calibration of T0 and sample position.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` minor interface update that allows fine control of bank rotation calibration.
- :ref:`PDCalibration <algm-PDCalibration-v1>` has a new option to use the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak function.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder-v1>` permits masking of discrete wavelength ranges to zero, for resonance filtering
- :ref:`SNAPReduce <algm-SNAPReduce-v1>` permits saving selected property names and values to file, to aid autoreduction.
- Add a custom ttmode to the PEARL powder diffraction scripts for running with a custom grouping file
- improve performance of :ref:`ApplyDiffCal <algm-ApplyDiffCal>` on large instruments eg WISH. This in turn improves the performance of :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`
- :ref:`LoadILLPolarizedDiffraction <algm-LoadILLPolarizedDiffraction>` now sorts the polarization orientations and enforces spin-flip, then non-spin-flip order
- :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` received a number of improvements: changes names of input workspaces to contain polarization information,
  transmission can be provided as a number or a workspace group, new data averaging option depending on measurement 2theta, option to display all measured points
  on a scatter plot, new option for self-attenuation treatment using measured tranmission.
- added a 3mf format file describing the PEARL sample and environment shapes for the P-E press. Also fixed a couple of minor issues in the 3mf file format loader used in ref:`LoadSampleEnvironment  <algm-LoadSampleEnvironment>`
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` now adds input run number also to a metadata field `run_list`, indended to contain a full list of numors, handled by :ref:`MergeRuns <algm-MergeRuns>`
- :ref:`LoadWANDSCD <algm-LoadWANDSCD-v1>` now has a new option to perform normalization in the same loading process.

Bugfixes
########
- Fix the issue with :ref:`SNSPowderReduction <algm-SNSPowderReduction>` - when invalid height unit is encountered while reading sample log, we should continue by ignoring geometry and rely purely on user input.
- fix d-spacing calculation when parabolic model is selected.
- Correct equation for pseudo-voigt FWHM and mixing parameter in peak profile function :ref:`Bk2BkExpConvPV <func-Bk2BkExpConvPV>`.

Engineering Diffraction
-----------------------
New features
############
- New setting for default peak function to fit in the Engineering Diffraction interface (initial default is :ref:`BackToBackExponential <func-BackToBackExponential>`).
- Added serial fit capability to fitting tab in EngDiff UI - this fits all loaded workspaces with same initial parameters.
- Add GSAS coefficients for parameters of peak profile function :ref:`Bk2BkExpConvPV <func-Bk2BkExpConvPV>` for ENGIN-X.
- Automatically subtract background from runs on loading in EngDiff UI.
- The most recently created or loaded Calibration is now selected by default in the load path when the interface is opened.
- The last used RB number is now saved for the next session
- The generation of the files required for Vanadium normalization is now done on the Focus tab of the user interface. This means the Vanadium data can be updated without
having to rerun the Ceria calibration. As part of this change the setting "Force Vanadium Recalculation" has been removed and the Vanadium run number input has been
moved from the Calibration tab to the Focus tab. The Vanadium run number is also no longer written to the prm generated on the Calibration tab (Note: this is a breaking
change and means .prm files generated from the EngDiff UI with older versions of Mantid won't load successfully)


Improvements
############
- The workflows for Calibration and Focusing in the EnggDiffraction GUI and EnginX scripts have been replaced to make use of faster, better tested C++ algorithms (PDCalibration) - as a result the following algorithms have been deprecated, and will likely be removed entirely in the next release: EnggCalibrate, EnggCalibrateFull, EnggFocus, EnggVanadiumCorrections.
- The cropping/region of interest selection for Calibration/Focusing is now chosen only on the Calibration tab, to avoid confusion and duplication of input.
- The region of interest for Calibration/Focusing can now be selected with a user-supplied custom calibration file.
- The Focused Run Files input box defaults to the last runs focused on the Focus tab, even if multiple runs were focussed
- The full calibration setting now has a default value consisting of the path to the ENGINX_full_instrument_calibration_193749.nxs file
- The usability of the file finder on the Fitting tab has been improved by the addition of file filters based on unit and/or bank

Bugfixes
########
- Sequential fitting in the EngDiff UI now uses the output of the last successful fit (as opposed to the previous fit) as the initial parameters for the next fit.
- An empty Engineering Diffraction interface is no longer saved if the user saves a project having previously had the interface open at some point in that session
- The help button on the Engineering Diffraction interface points to the correct page, having been broken in the last release
- Using the Clear button on the Workspace widget while using the Fitting tab no longer causes issues when you try to load runs back in.


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
- New definition file for D19 ILL instrument added.
- New algorithm :ref:`FindGlobalBMatrix <algm-FindGlobalBMatrix>` that refines common lattice parameters across peak workspaces from multiple runs with a different U matrix (which encodes the orientation) per run.

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
- :ref:`IndexPeaks <algm-IndexPeaks>` can now index peaks in a PeaksWorkspace with only a single run without optimising the UB (i.e. it is now possible to set CommonUBForAll=True in this instance).

:ref:`Release 6.2.0 <v6.2.0>`
