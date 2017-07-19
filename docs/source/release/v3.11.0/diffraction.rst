===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` now adjusts the sample offsets and has an option to optimize the initial time-of-flight for better calibration of single crystal data.

Engineering Diffraction
-----------------------

Powder Diffraction

- Added new diagrams showing the algorithms used in ISIS Powder scripts. These can be found at: :ref:`isis-powder-diffraction-workflow-ref`
- LoadILLAscii, which could be used to load D2B ASCII data into an MD workspace, has been removed. :ref:`LoadILLDiffraction <algm-LoadILLDiffraction>` should be used instead.

|

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
