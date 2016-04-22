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
- :ref:`LoadCIF <algm-LoadCIF>` can now also load structures where only anisotropic displacement parameters are given,
  which are converted to equivalent isotropic parameters.
- :ref:`SaveHKL <algm-SaveHKL>` has option to write the same output as anvred3.py including direction cosines.
- :ref:`LoadHKL <algm-LoadHKL>` reads hkl output that includes direction cosines.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` has DetCal information sorted by detector numbers

Engineering Diffraction
-----------------------

- New algorithm added: EnggFitDIFCFromPeaks, which forks from the old EnggFitPeaks.
  EnggFitPeaks modified to fit peaks but not calibration parameters.

Graphical user interface:

- Vanadium Curves and Ceria Peaks graphs are plotted once basic and cropped
  calibration process has been carried out
- Customise Bank Name text-field will set the workspace and .his file name
  according to this Bank Name provided by the user for Cropped Calibration
- The Fitting tab provides a graphical interface which fits an expected
  diffraction pattern and visualises them.
  The pastern is specified by providing a list of dSpacing values where Bragg
  peaks are expected. The algorithm :ref:`EnggFitPeaks<algm-EnggFitPeaks>`
  used in the background fit peaks in those areas using a peak fitting function.
- Fitting tab will automatically select and import all the focused bank files
  found within working directory and user can also select file now by providing
  a run-number.

- Improvements to the :ref:`Preview-Engineering_Diffraction-ref` section
  for Fitting tab, the zoom-in or zoom-out feature on to the data plot
  is enabled. As well as option to select peak, add peak or save peaks
  from the data plot is now supported.

- :ref:`Preview-Engineering_Diffraction-ref` under Fitting tab, you can
  now view the plot in `dSpacing` instead `ToF`, which enables you to
  rerun the fitting process after selecting peaks from the interface.


Imaging
-------

- The new algorithm :ref:`ImggAggregateWavelengths <algm-ImggAggregateWavelengths>`
  aggregates stacks of images from wavelength dependent data.

Improvements in the tomographic reconstruction graphical user interface:

- New capabilities added when visualizing stacks of images:

  - Handle the rotation of all the images in the stack
  - "Play" the stack or sequence of images as a movie
  - Visualize sample, flat, and dark images separately

- Previously existing parameters to set up local and remote paths have
  been moved into a new section of the interface. New options have
  been introduced for better flexibility. These are updated for the
  current infrastructure and are remembered between sessions.

- Normalization by flat and dark images can be disabled explicitly and
  separately, which is now supported in the underlying reconstruction
  scripts.

- The energy bands tab can now produce multiple output bands in one
  pass, and supports different aggregation methods via the new
  algorithm :ref:`ImggAggregateWavelengths
  <algm-ImggAggregateWavelengths>`.

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
