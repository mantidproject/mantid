===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

 - :ref:`algm-PredictPeaks` has a new option ``PredictPeaksOutsideDetectors`` which will predict peaks which fall outside of any defined detectors. This feature requires an extended detector space definition and will do nothing if this is not present in the IDF.
 - :ref:`algm-PredictPeaks` and `algm-FindPeaksMD` have a new option ``EdgePixels`` which will not predict or find peaks which fall in the input number of pixels from the edge of detectors.
 - :ref:`StartLiveData <algm-StartLiveData>` will load "live" data streaming from MaNDi data server.
 - :ref:`algm-PDCalibration <algm-PDCalibration>` is better at giving out physically meaningful results. It will no longer create calibrations that will convert time-of-flight to negative or imaginary d-spacing.
 - :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` now saves the calibration data for all detector banks in instrument so the header may be longer
 - :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` now uses the calibration lines to calibrate the detectors banks for CORELLI
 - :ref:SCD Event Data Reduction interface and SCD_Reduction python scripts work with both nxs and h5 extensions for data file.

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` Now supports supplying an a second ``.cal`` file for the ``GroupingFilename``.
- New algorithm :ref:`AlignAndFocusPowderFromFiles <algm-AlignAndFocusPowderFromFiles>` is a wrapper around :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` which supports caching results
- Bugfix in :ref:`SNAPReduce <algm-SNAPReduce>` with loading previous normalizations
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now supports splitters in format of ``MatrixWorkspace`` and general ``TableWorkspace``.
- A new NOMAD instrument definition file with corrected values.

Single Crystal Diffraction
--------------------------

- A new HB3A instrument definition file, for its 512 x 512 detector, is created.  Its valid period is from February 2017 to late April 2017.
- An IDF for HB3A with 256 by 256 detectors was created.  It was dated from late April 2017 because its original detector has been switched back.
- New algorithm :ref:`DeltaPDF3D <algm-DeltaPDF3D>` for calculating the 3D-deltaPDF from a HKL MDHistoWorkspace


Full list of `diffraction <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
