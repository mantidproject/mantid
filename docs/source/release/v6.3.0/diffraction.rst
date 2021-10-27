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
- Both :ref:`MultipleScatteringCorrection <algm-MultipleScatteringCorrection>` and :ref:`PaalmanPingsAbsorptionCorrection <algm-PaalmanPingsAbsorptionCorrection>` can use a different element size for container now.
- PEARL powder diffraction scripts now cope if absorption correction workspace is different size to the Vanadium workspace without generating NaN values

Improvements
############
- :ref:`FitPeaks <algm-FitPeaks>` and :ref:`PDCalibration <algm-PDCalibration>` no longer fit masked bins (bins with zero error).

Bugfixes
########
- :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>` now correctly writes all spectra to a single file when SplitFiles is False (previously wrote only a single spectrum).
- For processing vanadium run, we don't want to find environment automatically in :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>`.
- Restored behavior in :ref:`ConvertUnits <algm-ConvertUnits>` where negative time-of-flight converts to negative d-spacing when ``DIFA==0``
- Identification in :ref:`AlignComponents <algm-AlignComponents>` of the first and last detector-ID for an instrument component with unsorted detector-ID's.

Engineering Diffraction
-----------------------

New features
############
- Now support texture grouping (10 groups per bank) for ENGIN-X in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>`. Note this involved changes to the bankID log values saved with focused data, so this means the UI will not load in previously focused .nxs files.

Improvements
############
- Performance speed-up due to parallelisation when calibrating and focusing data into multiple groups in the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>`.
- Improved axes scaling in the plot of the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` :ref:`Fitting tab <ui engineering fitting>`.
- Automatically disable zoom and pan when opening the fit browser in the :ref:`Fitting tab <ui engineering fitting>` of the Engineering Diffraction interface (as they interfered with the interactive peak adding tool).
- The plot on the fitting tab is now made larger when undocked, unless the size of the overall interface has been expanded significantly.

Bugfixes
########
- Save .prm file from :ref:`Calibration tab <ui engineering calibration>` with correct L2 and two-theta for each group in arbitrary groupings (previously only correct for the two ENGIN-X banks).
- The last calibration file (.prm) populated in the :ref:`Calibration tab <ui engineering calibration>` is now correct when both banks are focused (previously was populated with just the South bank .prm)
- Fix crash on :ref:`Fitting tab <ui engineering fitting>` when trying to output fit results. The problem was caused by a unit conversion from TOF to dSpacing not being possible eg when peak centre at a negative TOF value


Single Crystal Diffraction
--------------------------
- Existing :ref:`PolDiffILLReduction <algm-PolDiffILLReduction>` and :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` can now reduce and properly normalise single-crystal data for the D7 ILL instrument.
- Enabling :ref:`SCDCalibratePanels <algm-SCDCalibratePanels-v2>` to calibrate each detector bank's size if it is a rectagular detector optionally.

:ref:`Release 6.3.0 <v6.3.0>`
