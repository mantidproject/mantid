===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` has parameter errors reduced, option for simplex minimization,
  and 3 new workspaces which can plot calculated vs theoretical columns, rows, and TOF for each bank.
- 5 detectors added to the MANDI instrument geometry

Engineering Diffraction
-----------------------

- Vanadium Curves and Ceria Peaks graphs are plotted once basic and cropped calibration process has been carried out
- Customise Bank Name text-field will set the workspace and .his file name according to this Bank Name
  provided by the user for Cropped Calibration
- The Fitting tab provides a graphical interface which fits an expected diffraction pattern and visualises them.
  The pastern is specified by providing a list of dSpacing values where Bragg peaks are expected. The algorithm
  :ref:`EnggFitPeaks<algm-EnggFitPeaks>` used in the background fit peaks in those areas using a peak fitting function.

Imaging
-------

Improvements in the tomographic reconstruction graphical user interface:

- Previously existing parameters to set up local and remote paths have
  been moved into a new section of the interface. New options have
  been introduced for better flexibility. These are updated for the
  current infrastructure and are remembered between sessions.

- Normalization by flat and dark images can be disabled explicitly and
  separately, which is now supported in the underlying reconstruction
  scripts.

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
