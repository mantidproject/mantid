===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

- LoadILLAscii, which could be used to load D2B ASCII data into an MD workspace, has been removed. :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` should be used instead.

|

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.

Powder Diffraction
------------------

- :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` now supports loading D2B data with detector scans. The D2B IDF has been updated, as previously it contained some errors in the positions of the tubes and size of the pixels.
