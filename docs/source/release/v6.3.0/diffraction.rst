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
* :ref:`StripVanadiumPeaks <algm-StripVanadiumPeaks-v2>` has 3 additional peak positions of 0.41192, 0.4279, 0.4907 angstroms.
- `GetDetOffsetsMultiPeaks`, which is deprecate since v6.2.0, is removed.
- `CalibrateRectangularDetectors`, which is deprecate since v6.2.0, is removed. And system test CalibrateRectangularDetectors_Test is removed.
- Extending :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` to the sample and container case.
- `absorptioncorrutils` now have the capability to calculate effective absorption correction (considering both absorption and multiple scattering).
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now has an option to manually specify sample geometry for absorption correction.
- Both :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` and :ref:`PaalmanPingsAbsorptionCorrection <algm-PaalmanPingsAbsorptionCorrection>` can use a different element size for container now.
- PEARL powder diffraction scripts now cope if absorption correction workspace is different size to the Vanadium workspace without generating NaN values
- :ref:`AnvredCorrection <algm-AnvredCorrection>` (and :ref:`SphericalAbsorption <algm-SphericalAbsorption>` which calls it) will now take the radius of a spherical sample from the workspace if the radius isn't specified (if the sample is not a sphere this will produce an error).
- :ref:`AnvredCorrection <algm-AnvredCorrection>` (and :ref:`SphericalAbsorption <algm-SphericalAbsorption>` which calls it) have been extended to evaluate the attenuation by more absorbing spherical samples (with muR < 9).
- :ref:`TotScatCalculateSelfScattering <algm-TotScatCalculateSelfScattering>` now groups the correction by detector bank in MomentumTransfer (rather than TOF)
  and includes the following update.
- :ref:`CalculatePlaczekSelfScattering v1 <algm-CalculatePlaczekSelfScattering-v1>` now validates that the IncidentSpectra
  is in units of Wavelength and will output in the same unit as the InputWorkspace. The parameter IncidentSpectra for :ref:`CalculatePlaczekSelfScattering <algm-CalculatePlaczekSelfScattering-v1>` has been
  renamed to fix a typo, which is a breaking change for this algorithm. Note that the addition of 1 to the Placzek correction has been moved out of this algorithm.

Improvements
############
- :ref:`FitPeaks <algm-FitPeaks>` and :ref:`PDCalibration <algm-PDCalibration>` no longer fit masked bins (bins with zero error).
- improve the Custom tt_mode in the ISIS PEARL powder diffraction scripts. Specifically the tt_mode Custom now supports all the different focus_modes if the grouping file contains 14 groups

Bugfixes
########
- :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>` now correctly writes all spectra to a single file when SplitFiles is False (previously wrote only a single spectrum).
- For processing vanadium run, we don't want to find environment automatically in :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>`.
- Restored behavior in :ref:`ConvertUnits <algm-ConvertUnits>` where negative time-of-flight converts to negative d-spacing when ``DIFA==0``
- Identification in :ref:`AlignComponents <algm-AlignComponents>` of the first and last detector-ID for an instrument component with unsorted detector-ID's.
- :ref:`LoadPDFgetNFile <algm-LoadPDFgetNFile>` now returns standard units for atomic distance rather than label
- Fix issue in :ref:`WANDPowderReduction <algm-WANDPowderReduction>` where in some cases you end up with zeros as output.
- Fix bug such that attenuation calculated in :ref:`AnvredCorrection <algm-AnvredCorrection>` is now accurate to within 0.5% for typical muR.
- The integration range has been corrected inside :ref:`PDFFourierTransform v2 <algm-PDFFourierTransform-v2>`.
- Fix problem with the create_vanadium action when running with tt_mode=Custom in the ISIS PEARL powder diffraction scripts. Create a separate Vanadium file for each different custom grouping file rather than one for all custom runs

Engineering Diffraction
-----------------------
New features
############
- Now support two texture grouping schemes: Texture20 (10 groups per bank, 20 in total) and Texture30 (15 groups per bank, 30 in total) for ENGIN-X in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>`. Note this involved changes to the bankID log values saved with focused data, so this means the UI will not load in previously focused .nxs files.

Improvements
############
- Performance speed-up due to parallelisation when calibrating and focusing data into multiple groups in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>`.
- Improved axes scaling in the plot of the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` :ref:`Fitting tab <ui engineering fitting>`.
- Automatically disable zoom and pan when opening the fit browser in the :ref:`Fitting tab <ui engineering fitting>` of the Engineering Diffraction interface (as they interfered with the interactive peak adding tool).
- The plot on the fitting tab is now made larger when undocked, unless the size of the overall interface has been expanded significantly.
- :ref:`FilterEvents <algm-FilterEvents>` execution speed improved by 35% in some cases.
- Updated the default values for :ref:`EnggEstimateFocussedBackground <algm-EnggEstimateFocussedBackground>` and in the fitting tab table to Niter = 50 and XWindow = { 600 for TOF, 0.02 for dSpacing }.
- The file filter in the Focus tab for calibration Region includes "No Region Filter", North, South and now also Cropped, Custom, Texture and Both Banks. The text for "No Unit/Region Filter" are colored grey.
- The fitting tab has been made more tolerant to users deleting or renaming the workspaces in the workbench Workspaces widget.

Bugfixes
########
- Save .prm file from :ref:`Calibration tab <ui engineering calibration>` with correct L2 and two-theta for each group in arbitrary groupings (previously only correct for the two ENGIN-X banks).
- The last calibration file (.prm) populated in the :ref:`Calibration tab <ui engineering calibration>` is now correct when both banks are focused (previously was populated with just the South bank .prm)
- Fix crash on :ref:`Fitting tab <ui engineering fitting>` when trying to output fit results. The problem was caused by a unit conversion from TOF to dSpacing not being possible eg when peak centre at a negative TOF value
- The Serial and Sequential fit features on the Fitting tab now respect the "Subtract BG" checkbox in the table and use the background subtracted workspace where this is checked

Single Crystal Diffraction
--------------------------
- Existing :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` and :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` can now reduce and properly normalise single-crystal data for the D7 ILL instrument.
- Enabling :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` to calibrate each detector bank's size if it is a rectagular detector optionally.
- Fixed calculation of modulation vector uncertainty in :ref:`FindUBUsingIndexedPeaks <algm-FindUBUsingIndexedPeaks>`, new option ``CommonUBForAll`` allow selection of calculation handling multiple run the same as :ref:`IndexPeaks <algm-IndexPeaks>`.

Bugfixes
########
- :ref:`ConvertWANDSCDtoQ<algm-ConvertWANDSCDtoQ>` and :ref:`ConvertQtoHKLMDHisto<algm-ConvertQtoHKLMDHisto>` units now display correctly in terms of 'in X.XXX A^-1'
- :ref:`ConvertQtoHKLMDHisto<algm-ConvertQtoHKLMDHisto>` output orientation fixed
- :ref:`SaveReflections <algm-SaveReflections>` now scales intensities and errors to ensure the width of the columns in the output file are not exceeded.
:ref:`Release 6.3.0 <v6.3.0>`
