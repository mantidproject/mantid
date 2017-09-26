===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` now adjusts the sample offsets and has an option to optimize the initial time-of-flight for better calibration of single crystal data.
- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` has CalibrateSnapPanels option to calibrate 3X3 banks of SNAP instrument for single crystal data.
- :ref:`LoadIsawDetCal <algm-LoadIsawDetCal>` has not correctly aligned the detectors for SNAP since release 3.9. This bug that only impacted SNAP has been fixed.

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

- Added new diagrams showing the algorithms used in ISIS Powder scripts. These can be found at: :ref:`isis-powder-diffraction-workflow-ref`
- LoadILLAscii, which could be used to load D2B ASCII data into an MD workspace, has been removed. :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` should be used instead.
- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` now supports loading D2B data with detector scans. The D2B IDF has been updated, as previously it contained some errors in the positions of the tubes and size of the pixels.
- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` now correctly supports overloading the grouping file in the presence of a masking workspace.
- New algorithm :ref:`Bin2DPowderDiffraction <algm-Bin2DPowderDiffraction>` to bin TOF powder diffraction event data in :math:`(d,\,d_{\perp})` space has been developed.
- :ref:`PDCalibration <algm-PDCalibration>` has changed how it calculates constants from peak positions to use a simplex optimization rather than Gauss-Markov method.
- :ref:`ResampleX <algm-ResampleX>` has a bug fix in how it automatically determines the data range for a workspace with multiple spectra.
- The powder diffraction GUI has had numerous bugfixes and now has an option to override the detector grouping.

Single Crystal Diffraction
--------------------------

- New algorithm :ref:`SingleCrystalDiffuseReduction <algm-SingleCrystalDiffuseReduction>` which performs the most common reductions done on Corelli (and elsewhere) for single crystal diffuse scattering.
- New algorithm :ref:`ConvertMultipleRunsToSingleCrystalMD <algm-ConvertMultipleRunsToSingleCrystalMD>` which loads, converts to single crystal MDWorkspace and combines a series of runs.
- :ref:`FindPeaksMD <algm-FindPeaksMD>` has been modified to only add peaks to runs that contributed to that peak. This is a lot faster when multiple runs are in the same MDworkspace.
- New algorithm :ref:`MDNormSCDPreprocessIncoherent <algm-MDNormSCDPreprocessIncoherent>` creates the Solid Angle and Flux workspace from Vanadium data for MDNormSCD
- :ref:`FindSXPeaks <algm-FindSXPeaks-v1>` now finds all peaks in each spectrum. It also allows for setting more fine-grained resolutions. It can now also accept workspaces both in units of TOF and d-spacing.

Powder Diffraction
------------------

- New set of routines for HRPD (:ref:`isis-powder-diffraction-hrpd-ref`), designed to replace and improve on the old CRI scripts

Imaging
-------

- The IMAT IDF has been improved to more accurately represent the instrument.


Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.

