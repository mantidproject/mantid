===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Improvements
############

- :ref:`LoadILLTOF2 <algm-LoadILLTOF-v2>` now supports data from PANTHER used in diffraction mode.
- A unit `AtomicDistance` measured in units of Angstrom has been added to the unit factory to describe atomic structure.

Powder Diffraction
------------------

- The create_total_scattering_pdf merging banks now matches spectra to the spectrum with the largest x range.
- The create_total_scattering_pdf merging banks no longer matches spectra with scale, it now only matches with offset.
- The Polaris create_total_scattering_pdf can now be given an parameter `output_binning` that will be used to rebin the output_pdf.
- The polaris create_total_scattering_pdf function can now accept a `pdf_type` argument to set the pdf_output type.
- :ref:`HRPDSlabCanAbsorption <algm-HRPDSlabCanAbsorption-v1>` now accepts any thickness parameter and not those in a specified list.

Engineering Diffraction
-----------------------

- :ref:`EnggCalibrateFull <algm-EnggCalibrateFull-v1>` now uses the new :ref:`SaveAscii <algm-SaveAscii-v2>` to save its output files as TSVs, allowing them to be loaded back into mantid.
- :ref:`FitPeaks <algm-FitPeaks-v1>` allows for specifying some of the peak parameters (e.g. only ``Mixing`` for :ref:`func-PseudoVoigt`) and observing the other parameters. The new functionality allows for more automated execution for peak functions other than :ref:`func-Gaussian`.

Single Crystal Diffraction
--------------------------

- :ref:`PredictFractionalPeaks <algm-PredictFractionalPeaks-v1>` now accepts the same set of modulation vector properties as :ref:`IndexPeaks <algm-IndexPeaks-v1>`.
- New algorithm :ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE-v1>` for converting HFIR single crystal data (from WAND and DEMAND) into MDEventWorkspace in units Q_sample.
- ``IndexPeaksWithsatellites`` has been deleted as it had been deprecated and superseded by :ref:`IndexPeaks <algm-IndexPeaks-v1>`.
- The output peak workspace from :ref:`PredictFractionalPeaks<algm-PredictFractionalPeaks-v1>` now keeps the same lattice parameters as the input workspace. 
- :ref:`SaveReflections <algm-SaveReflections>` now has the option to save peaks to separate files based on their associated modulation vectors
  when using the Jana format.

Imaging
-------

BugFixes
########

Powder Diffraction
------------------

- A bug has been fixed that prevented unicode strings being given as a lim file directory in polaris create_total_scattering_pdf merging banks.
- A bug has been fixed that caused Polaris.focus to fail with `do_absorption_Corrections=True`.
- A bug has been fixed that caused empty runs to be subtracted twice when specifying `sample_empty` in `Polaris.focus`.

Engineering Diffraction
-----------------------

- Fixed a bug where `SaveGSS <algm-SaveGSS-v1>` could crash when attempting to pass a group workspace into it.

Single Crystal Diffraction
--------------------------

- Support added for DEMAND (HB3A) to the algorithms :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ-v1>` and :ref:`FindPeaksMD <algm-FindPeaksMD-v1>` in order to handle additional goniometers.
- Fixed PredictSatellitePeaks producing an empty table when using cross-terms with crystallography convention for sign of Q.

Imaging
-------

:ref:`Release 4.3.0 <v4.3.0>`
