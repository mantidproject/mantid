===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Documentation
-------------

- The documentation for all calibration approaches, including Powder diffrction, single crystal and engineering calibrations has been pulled together, and expanded :ref:`here<Calibration>`.

Crystal Improvements
--------------------

- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` has parameter errors reduced,
   option for simplex minimization, and 3 new workspaces which can plot calculated 
   vs theoretical columns, rows, and TOF for each bank. Calibration is now as good 
   as ISAW's for Mandi data.
- 5 detectors added to the MANDI instrument geometry
- :ref:`LoadCIF <algm-LoadCIF>` can now also load structures where only anisotropic displacement parameters are given,
  which are converted to equivalent isotropic parameters.
- :ref:`SaveHKL <algm-SaveHKL>` has option to write the same output as anvred3.py including direction cosines.
- :ref:`LoadHKL <algm-LoadHKL>` reads hkl output that includes direction cosines.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` has DetCal information sorted by detector numbers
- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` has resolution shells in units of d-Spacing.


Engineering Diffraction
-----------------------

- New algorithm added:
  :ref:`EnggFitDIFCFromPeaks<algm-EnggFitDIFCFromPeaks>`, which forks
  from the old
  :ref:`EnggFitPeaks<algm-EnggFitPeaks>`. :ref:`EnggFitPeaks<algm-EnggFitPeaks>`
  modified to fit peaks but not calibration parameters.

- An option to set the initial rebin width has been added to
  :ref:`EnggCalibrateFull<algm-EnggCalibrateFull>`

- :ref:`EnggFocus<algm-EnggFocus>` now has an option to mask out
  several ranges in ToF (instrument pulses), with default values set
  for ENGIN-X, and an option to normalize by proton charge (enabled by
  default).

- Phase information files for ENGIN-X are now distributed together
  with the ENGIN-X scripts.

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


- The output calibration files will be written with the extension
  ".prm" by default. Together with an "all-banks" calibration (GSAS
  instrument parameters) file, an individual file for every focused
  bank will be written.

- Improvements to Fitting tab, Single peak fitting for consecutive run
  number's focused files has now been enabled. The tab will automatically load
  the consecutive run numbers to the list widget from the range provided by the
  user, bank combo-box will update upon selection of run number.


- New algorithm :ref:`SaveNexusPD <algm-SaveNexusPD>` which creates a nexus file for use in GUDRUN and will hopefully be supported by Rietveld packages in the future.

Imaging
-------

- The new algorithm :ref:`ImggAggregateWavelengths <algm-ImggAggregateWavelengths>`
  aggregates stacks of images from wavelength dependent data.

- Images loaded as Mantid workspaces can now be saved into FITS files
  using the algorithm :ref:`SaveFITS <algm-SaveFITS>`.

Improvements in the tomographic reconstruction graphical user interface:

- New capabilities added when visualizing stacks of images:

  - Handle the rotation of all the images in the stack
  - "Play" the stack or sequence of images as a movie
  - Visualize sample, flat, and dark images separately
  - Multiple color map alternatives and control of the color bar

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


Powder Diffraction
------------------

- :ref:`PDFFourierTransform <algm-PDFFourierTransform>` has been corrected in its calculation of errors.


Powder Diffraction Scripts
--------------------------

- Pearl powder diffraction has been integrated and can be found
  `scripts/PearlPowderISIS`. The routines/script has been differentiated from
  the long list of directories of calibration and raw files. The calibration
  directories can be found in a file by the name of pearl_calib_factory.py,
  whereas the raw directories can be found in a file by the name of
  pearl_cycle_factory.py.

- PowderISIS script has been renamed to CryPowderISIS and can be found within
  the following folder `scripts/CryPowderISIS`

- :ref:`pearl-powder-diffraction-ref` documentation has been implemented and
  PowderISIS script documentation has been renamed to
  :ref:`cry-powder-diffraction-ref`

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
