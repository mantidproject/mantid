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

- :ref:`IntegratePeaksMDHKL <algm-IntegratePeaksMDHKL>` has been added to integrate data in HKL space.  The
  main usage will be to normalize the data using
  :ref:`MDNormSCD <algm-MDNormSCD>` and then integrate the resulting MDHistoWorkspace,
  but it also integrates MDHistoWorkspaces and MDEventWorkspaces without normalizing.
  The MD data must be in units of HKL.  A 3D box is created for each peak and the background
  and peak data are separated.  The intensity and sigma of the intensity is found from the grid inside the peak and
  the background is subtracted.  The boxes are created and integrated in parallel and less memory is required than
  binning all HKL space at once. The figure shows the grid points within an HKL box that are in one peak from Si data.

.. figure::  ../../images/peak3d.png
   :width: 487
   :align: center


- Some improvements were done for creating peaks from python. :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>`
  copies the goniometer from the input MatrixWorkspace to PeaksWorkspace. createPeak for PeaksWorkspace copies goniometer
  from PeaksWorkspace to peak. setGoniometer for a peak can be done from python and setQLabFrame and setQSampleFrame works
  correctly now with one argument.


Engineering Diffraction
-----------------------

- EnggFocus: bins are now masked at the beginning of the workflow
  (when using the option MaskBinsXMins)

- :ref:`SaveDiffFittingAscii <algm-SaveDiffFittingAscii>` an algorithm which saves a TableWorkspace containing
  diffraction fitting results as an ASCII file


Powder Diffraction
------------------

- :ref:`SNSPowderReduction <algm-SNSPowderReduction>` has changed
  parameters. ``Instrument``, ``RunNumber``, and ``Extension`` have
  been replaced with a single ``Filename`` parameter. This has been
  paired with changes to the Powder Diffraction interface as
  well. There were also a variety of bugfixes related to the output
  workspaces. While it did not affect the saved data files, the output
  workspaces were not always correctly normalized or in the requested
  units.

- :ref:`PDFFourierTransformSNSPowderReduction
  <algm-PDFFourierTransformSNSPowderReduction>` has been modified to
  look at the signal as well when looking at the ``Q``-range to use
  for the transform.

- :ref:`cry-powder-diffraction-ref`: S-Empty option has been enabled for
   the Crystallography Powder Diffraction Script. In order to use the
   S-Empty option, simply provide the S-Empty run number within the
   ``.pref`` file.

- :ref:`CorelliCrossCorrelate <algm-CorelliCrossCorrelate>`: The
  weights applied to events have changed by a factor of the duty cycle
  (:math:`c\approx0.498`) as requested by the instrument scientists.
  
- :ref:`pearl-powder-diffraction-ref`: A workflow diagram for 
  ``pearl_run_focus`` function has been created. 

Imaging
-------

Tomographic reconstruction graphical user interface
###################################################

- Fixed the submission of custom commands.



Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
