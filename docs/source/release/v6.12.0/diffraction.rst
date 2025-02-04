===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- The ability to define the output binning of each spectra has been added to :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>`.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder-v1>` has been modified to utilize the new binning parameters added to :ref:`DiffractionFocussing <algm-DiffractionFocussing-v2>`.
- New container type added in for POWGEN instrument and accordingly the :ref:`SNSPowderReduction <algm-SNSPowderReduction>` interface has been updated to accommodate this new container type.
- New focus mode "trans_custom" added to :ref:`PEARL powder <isis-powder-diffraction-pearl-ref>` routine which allows a user to specify modules to include in the transverse bank focusing using the parameter trans_mod_nums. The module numbers in the range 1-9 can be specified using the same string syntax as run-numbers - e.g. trans_mod_nums="1-3,5" corresponds to focusing modules 1,2,3 and 5.
- Move `pystog <https://github.com/neutrons/pystog>`_ from v0.2.7 to v0.5.0 which supports numpy v2
-  Added new mode ``mode="pdf_norm"`` to :ref:`ISIS POLARIS powder reduction<isis-powder-diffraction-polaris-ref>` for when the chopper is off/stationary with a default ``van_normalisation_method="absolute"``. The existing  ``mode="pdf"`` now has a default  ``van_normalisation_method="relative"`` (was previously ``"absolute"``).
- Add more information to :ref:`CrossCorrelate <algm-CrossCorrelate>` and :ref:`GetDetectorOffsets <algm-GetDetectorOffsets>` user documenation

Bugfixes
############
- Fix bug where unable to call `create_vanadium` without setting sample details in :ref:`ISIS Powder Diffraction Scripts <isis-powder-diffraction-ref>`.
- Fix the issue with the :ref:`InterpolateBackground <algm-InterpolateBackground>` algorithm.


Engineering Diffraction
-----------------------

New features
############


Bugfixes
############
- Fix Deprecation Warning for getSampleDetails in :ref:`Fitting tab <ui engineering fitting>`
- Fix issue with :ref:`LoadEventNexus <algm-LoadEventNexus>` loading banks 1, 5, 6 as single banks


Single Crystal Diffraction
--------------------------

New features
############
- Updated :ref:`FindGlobalBMatrix <algm-FindGlobalBMatrix-v1>` to change how reference UBs are chosen before refinement and improvements made to verbosity of error/ warning reporting for users.
- Added table view for group peaks workspaces, displaying group indices alongside standard peak data, with all the capabilities of a standard table view.
- A new peak shape named `detectorbin` was introduced to save the detector IDs and bin indices of either TOF or dSpacing domains. This peak shape could be used for Overlap detection, Two-step integration and Eventual visualisation on instrument view.
- The new peak shape has been associated with the peaks integrated with :ref:`algm-IntegratePeaksShoeboxTOF` and :ref:`IntegratePeaksSkew <algm-IntegratePeaksSkew>` integration algorithms.
- By accessing the `detectorbin` peak shape users now can view the detector ids associated for each integrated peak from the above algorithms.
- Small usability improvements to :ref:`ISIS Single Crystal Diffraction Reduction Classes <isis-single-crystal-diffraction-ref>`:

  - Allow users to pass key-word arguments to methods ``save_peak_table`` and ``save_all_peaks`` (passed to :ref:`SaveReflections<algm-SaveReflections-v1>`)
  - Make saving of .nxs file of peak tables in above methods optional using argument ``save_nxs`` (default is True)
  - Check UB filepath exists in method ``load_isaw_ub``
  - Added option to set min I/Sigma ``min_intens_over_Sigma`` in method ``remove_non_integrated_peaks`` (default is 0)
- Add option ``SeparateBatchNumbers`` to :ref:`SaveReflections <algm-SaveReflections-v1>` to write a different batch number/scale factor ID for each run.
- Use :ref:`MultiDomainFunction<func-MultiDomainFunction>` in :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` to tie peak profile parameters accross pixels.

Bugfixes
############
- Fix bug in reflection condition affecting all-face centered lattice

:ref:`Release 6.12.0 <v6.12.0>`
