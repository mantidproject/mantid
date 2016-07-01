===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------

Single Crystal Diffraction
--------------------------

- HFIR HB3A's data reduction interface application (MantidPlot/Interfaces/Diffraction/HFIR 4Circle Reduction)
  has been expanded and improved from previous release. It provides an integrated user-friendly interface for
  instrument scientists and users to access data, calculate and refine UB matrix, merge multiple data sets
  for slice-view and peak integration.
.. figure::  ../../images/peak3d.png
      :width: 487
   :align: right
- :ref:`IntegratePeaksMDHKL <algm-IntegratePeaksMDHKL>` has been added to integrate data in HKL space.  The 
  main usage will be for data normalized by :ref:`MDNormSCD <algm-MDNormSCD>`, but it also works for MDHistoWorkspaces
  and MDEventWorkspaces.  The MD data must be in units of HKL.  A 3D box is created for each peak and the background
  and peak data are separated.  The intensity and sigma of the intensity is found from the grid inside the peak and
  the background is subtracted.  The boxes are created and integrated in parallel and less memory is required than
  binning all HKL at once.
  

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has changed
  parameters. ``Instrument``, ``RunNumber``, and ``Extension`` have
  been replaced with a single ``Filename`` parameter. This has been
  paired with changes to the Powder Diffraction interface as well. An
  additional parameter, ``LogFilename``, has been added to aid in
  testing files being produced by the "new DAS" at SNS. This parameter
  has not been added to the Powder Diffraction interface and will be
  removed without notice.

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
