===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------
-  :ref:`PredictPeaks <algm-PredictPeaks>`
   has been modified to optionally use the point group symmetry
   so that HKL peaks that are equivalent, due to symmetry, are
   considered to be the same.  Only the primary hkl are predicted
   which means that higher symmetries will predict fewer peaks.
- :ref:`StartLiveData <algm-StartLiveData>` will load "live"
  data streaming from MaNDi data server.

Engineering Diffraction
-----------------------

|

Full list of `diffraction <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <https://github.com/mantidproject/mantid/issues?q=is%3Aclosed+milestone%3A%22Release+3.10%22+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
