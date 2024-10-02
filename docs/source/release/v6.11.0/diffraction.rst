===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- Added option to :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` to support logarithmic compression
- :ref:`LoadEventAndCompress <algm-LoadEventAndCompress>` has been updated to take advantage of new capability in LoadEventNexus
- Add a minimum signal-to-sigma post-fitting check to :ref:`FitPeaks <algm-FitPeaks>` and :ref:`PDCalibration <algm-PDCalibration>`, where peaks with a signal below the provided threshold will be rejected.
- Add option ``fit_prompt_pulse`` to HRPD (ISIS) reduction to fit and subtract prompt pulse (as opposed to default method of masking of prompt pulse in TOF).
- :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` has been updated to take advantage of new capability in :ref:`LoadEventNexus <algm-LoadEventNexus>` and :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has been updated to take advantage of new capabilities in :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` and :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`

Bugfixes
############
- Fix bug where compression isn't run when logarithmic is selected in :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder-v1>`
- Refine error message when applying absorption correction with missing sample details in powder diffraction scripts
- Fix bug in :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>` not properly accumulating data when :ref:`CompressEvents <algm-CompressEvents-v1>` prior to it in ``PreserveEvents=False`` mode
- Defect since 6.9.0: fix bug in PDCalibration where input pixels with empty event lists were no longer being masked
- Fix bug in the calibration diagnostics plotting where the solid angle of detectors were not extracted successfully
- Fix bug in POLARIS (ISIS) routine ``create_total_scattering_pdf`` causing unhandled error when calculating cross-sections of multi-atom unit cells for pdf normalisation.


Engineering Diffraction
-----------------------

New features
############


Bugfixes
############
- Fixed a crash in :ref:`Fitting tab <ui engineering fitting>` of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` when a fit is re-run for a function like PseudoVoigt that contains FWHM parameter as the peak function.


Single Crystal Diffraction
--------------------------

New features
############
- Improve determination of background bins by minimising third-moment (skew) in :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` by forcing skew > 0 (minimum skew would expect in background)
- Add cabability to not integrate peaks which include a masked detector in the following algorithms

  - :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`
  - :ref:`IntegratePeaksShoeboxTOF <algm-IntegratePeaksShoeboxTOF>`
  - :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>`
- Add method ``find_consistent_ub`` to ISIS single-crystal reduction classes to find a UB that preserves indexing given a reference workspace (at different goniometer angle(s))
- Add method ``optimize_goniometer_axis`` to ISIS single-crystal reduction classes to optimise the goniometer axes and angles given a sequence of workspaces at different goniometer angle with consistent UBs
- Update to algorithm :ref:`AddAbsorptionWeightedPathLengths <algm-AddAbsorptionWeightedPathLengths>` that now allows lean peaks to be used. There is also an option to apply the correction.
- New algorithm :ref:`FindMultipleUMatrices <algm-FindMultipleUMatrices>` to find multiple UB matrices (given lattice parameters) for sample with multiple domains or spurious peaks.
- Add method `calc_absorption_weighted_path_lengths` to ISIS single-crystal reduction classes that calculate tbar for each peak (saved in a column of the table) and optionally apply an attenuation correction to the integrated intensity of each peak. By default the correction will be applied if class has property scale_integrated = True)
- A new output property, ``Cells``, has been added to :ref:`ShowPossibleCells <algm-ShowPossibleCells>` that includes the cell information in a usable way
- New algorithm :ref:`SaveMDHistoToVTK <algm-SaveMDHistoToVTK>` that saves a MDHistoWorkspace as a VTK file so that it can be visualized by Paraview.
- Improve shoebox position optimisation in :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` - would previously be centred on nearby stronger peaks if present.

Bugfixes
############
- :ref:`LoadWANDSCD <algm-LoadWANDSCD>` now keeps the sgl/sgu angles set by the goniometer
- :ref:`ConverWANDSCDToQ <algm-ConvertWANDSCDToQ>` simplifies the underlying code by using histograms directly

:ref:`Release 6.11.0 <v6.11.0>`