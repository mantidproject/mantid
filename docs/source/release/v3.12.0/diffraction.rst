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

- Some new functionality for POLARIS in the ISIS Powder scripts. Adjusted some default parameters and output unsplined vanadium workspace by default
- ISIS_Powder scripts for PEARL now support creation of grouping .cal files from ceria run(s)
- The ``CalibrationFile`` is now optional in :ref:`SNSPowderReduction <algm-SNSPowderReduction>`. In this case time focussing will use :ref:`ConvertUnits <algm-ConvertUnits>` and the instrument geometry. Care must be taken to supply a ``GroupingFile`` otherwise all of the spectra will be kept separate.
- New algorithm :ref:`algm-EstimateDivergence` estimates the beam divergence due to finite slit size
- :ref:`PDCalibration <algm-PDCalibration>` returns three more diagnostic workspaces: one for the fitted peak heights, one for the fitted peak widths, and one for observed resolution.
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` now supports D2B files with calibrated data.

Engineering Diffraction
-----------------------

- Usability improvements in the GUI:
  - The "Invalid RB number" popup window in the GUI has been replaced with a more user-friendly message
  - Improved progress reporting for Calibration and Focus

Single Crystal Diffraction
--------------------------
- :ref:`FilterPeaks <algm-FilterPeaks>` now supports filtering peaks by TOF, d-spacing, and wavelength.

- HB3A reduction interface has been enhanced.  A child window is added to it for users to pre-process scans and save the processed and merged data to NeXus files in order to save time when they start to reduce and visualize the data.
- :ref:`IntegratePeaksMDHKL <algm-IntegratePeaksMDHKL>` now has option to specify background shell instead of using default background determination.


Imaging
-------

:ref:`Release 3.12.0 <v3.12.0>`
