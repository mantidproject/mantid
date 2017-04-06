===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

 - :ref:`algm-PredictPeaks` has a new option ``PredictPeaksOutsideDetectors`` which will predict peaks which fall outside of any defined detectors. This feature requires an extended detector space definition and will do nothing if this is not present in the IDF.
 - :ref:`StartLiveData <algm-StartLiveData>` will load "live" data streaming from MaNDi data server.

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

- :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` Now supports supplying an a second ``.cal`` file for the ``GroupingFilename``.
- Bugfix in :ref:`SNAPReduce <algm-SNAPReduce>` with loading previous normalizations
- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` now supports splitters in format of ``MatrixWorkspace`` and general ``TableWorkspace``.
- A new NOMAD instrument definition file with corrected values.

Single Crystal Diffraction
--------------------------

- A new HB3A instrument definition file, for its 512 x 512 detector

Full list of `diffraction <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
