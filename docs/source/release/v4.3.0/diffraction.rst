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

Powder Diffraction
------------------

- The create_total_scattering_pdf merging banks now matches spectra to the spectrum with the largest x range.
- The create_total_scattering_pdf merging banks no longer matches spectra with scale, it now only matches with offset.
- :ref:`HRPDSlabCanAbsorption <algm-HRPDSlabCanAbsorption-v1>` now accepts any thickness parameter and not those in a specified list.

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

- :ref:`PredictFractionalPeaks <algm-PredictFractionalPeaks-v1>` now accepts the same set of modulation vector properties as :ref:`IndexPeaks <algm-IndexPeaks-v1>`.
- New algorithm :ref:`ConvertHFIRSCDtoMDE <algm-ConvertHFIRSCDtoMDE-v1>` for converting HFIR single crystal data (from WAND and DEMAND) into MDEventWorkspace in units Q_sample.

Imaging
-------

BugFixes
########

Powder Diffraction
------------------

- A bug has been fixed that prevented unicode strings being given as a lim file directory in polaris create_total_scattering_pdf merging banks.

Engineering Diffraction
-----------------------

- Fixed a bug where `SaveGSS <algm-SaveGSS-v1>` could crash when attempting to pass a group workspace into it.

Single Crystal Diffraction
--------------------------

- Support added for DEMAND (HB3A) to the algorithms :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ-v1>` and :ref:`FindPeaksMD <algm-FindPeaksMD-v1>` in order to handle additional goniometers.

Imaging
-------

:ref:`Release 4.3.0 <v4.3.0>`
