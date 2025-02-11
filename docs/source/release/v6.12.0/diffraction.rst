===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>` now allows the user to define binning parameters for each output spectrum.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder-v1>` uses the new binning parameters added to :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>`.
- New powder cell container type added for POWGEN. :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now accommodates this container type.
- :ref:`PEARL powder <isis-powder-diffraction-pearl-ref>` has a new ``trans_custom`` focus mode. This allows the user to specify modules to include in the transverse bank focusing using the parameter ``trans_mod_nums``. The module numbers in the range 1-9 can be specified using the same string syntax as run-numbers - e.g. ``trans_mod_nums="1-3,5"`` corresponds to focusing modules 1,2,3 and 5.
- `pystog <https://github.com/neutrons/pystog>`_ upgraded from v0.2.7 to v0.5.0, which supports NumPy v2.
- :ref:`ISIS POLARIS powder reduction<isis-powder-diffraction-polaris-ref>` has a new mode, ``mode="pdf_norm"``, for when the chopper is off/stationary with a default ``van_normalisation_method="absolute"``. The existing  ``mode="pdf"`` now has a default  ``van_normalisation_method="relative"`` (was previously ``"absolute"``).
- :ref:`CrossCorrelate <algm-CrossCorrelate>` and :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>` user documentation improvements.

Bugfixes
############
- In :ref:`ISIS Powder Diffraction Scripts <isis-powder-diffraction-ref>`, ``create_vanadium`` now works without setting the sample material and geometry/shape (this information is retrieved from the advanced config files for each instrument). 
- :ref:`InterpolateBackground <algm-InterpolateBackground>` now works as expected.


Engineering Diffraction
-----------------------

Bugfixes
############
- ``getSampleDetails`` deprecation warning resolved in the :ref:`fitting tab <ui engineering fitting>`.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` is now able to load banks 1, 5, 6 as single banks for VULCAN.


Single Crystal Diffraction
--------------------------

New features
############
- Updated :ref:`FindGlobalBMatrix <algm-FindGlobalBMatrix-v1>` to change how reference UBs are chosen before refinement. Output of error and warning messages has also been improved.
- Added a table view for groups of peaks workspaces, displaying group indices alongside standard peak data, with all the capabilities of a standard table view.
- ``PeakShapeDetectorBin`` peak shape introduced to store the detector IDs and bin indices for either TOF or d-spacing units. This peak shape is stored on the ``Peak`` object after using the :ref:`algm-IntegratePeaksShoeboxTOF` and :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>`  algorithms.
- :ref:`ISIS Single Crystal Diffraction Reduction Classes <isis-single-crystal-diffraction-ref>` usability improvements:

  - ``save_peak_table`` and ``save_all_peaks`` methods now accept keyword arguments (passed to :ref:`SaveReflections<algm-SaveReflections-v1>`).
  - ``save_nxs=False`` can now be passed as an optional key word argument to the above functions to turn off saving .nxs peak tables (default is True).
  - ``load_isaw_ub`` now checks whether the UB file path exists before attempting to run the ``LoadIsawUB`` algorithm.
  - ``remove_non_integrated_peaks`` now has the optional argument ``min_intens_over_sigma`` to set min I/Sigma (default is 0).
- :ref:`SaveReflections <algm-SaveReflections-v1>` now has the option ``SeparateBatchNumbers`` to write a different batch number/scale factor ID for each run.
- :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` now uses :ref:`MultiDomainFunction<func-MultiDomainFunction>` to tie peak profile parameters across pixels.

Bugfixes
############
- Corrected logic in the all-face centred reflection condition that would cause an occasional crash in :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>`.
- :ref:`SaveINS<algm-SaveINS-v1>` now saves only the minimum required symmetry operations to file.

:ref:`Release 6.12.0 <v6.12.0>`
