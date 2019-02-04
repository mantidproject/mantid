===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Documentation
-------------
 
- The documentation for all calibration approaches, including Powder diffraction, single crystal and engineering calibrations has been pulled together, and expanded :ref:`here <Calibration Intro>`. 

Powder Diffraction
------------------

- :ref:`PDFFourierTransform <algm-PDFFourierTransform>` has been corrected in its calculation of errors.
- New algorithm: :ref:`PDToGUDRUN <algm-PDToGUDRUN>` loads and processes data for use in `GUDRUN <http://www.isis.stfc.ac.uk/instruments/sandals/data-analysis/gudrun8864.html>`_.

Powder Diffraction Scripts
##########################

- Pearl powder diffraction has been integrated and can be found
  `scripts/PearlPowderISIS`. The routines/script has been differentiated from
  the long list of directories of calibration and raw files. The calibration
  directories can be found in a file by the name of pearl_calib_factory.py,
  whereas the raw directories can be found in a file by the name of
  pearl_cycle_factory.py.

- PowderISIS script has been renamed to CryPowderISIS and can be found within
  the following folder `scripts/CryPowderISIS`

- `Pearl Powder Diffraction Script <http://docs.mantidproject.org/v3.7.1/api/python/techniques/PearlPowderDiffractionISIS-v1.html>`_ 
  documentation has been implemented and
  PowderISIS script documentation has been renamed to
  `Crystallography Powder Diffraction Script <http://docs.mantidproject.org/v3.7.1/api/python/techniques/CryPowderDiffractionISIS-v1.html>`_

Single Crystal Improvements
---------------------------

- :ref:`SCDCalibratePanels <algm-SCDCalibratePanels>` has parameter errors reduced,
  an option for simplex minimization, and 3 new workspaces which can plot calculated 
  vs theoretical columns, rows, and TOF for each bank. Calibration is now as good 
  as ISAW's for Mandi data.
- 5 detectors added to the MANDI instrument geometry
- :ref:`LoadCIF <algm-LoadCIF>` can now also load structures where only anisotropic displacement parameters are given,
  which are converted to equivalent isotropic parameters.
- :ref:`SaveHKL <algm-SaveHKL>` has option to write the same output as anvred3.py including direction cosines.
- :ref:`LoadHKL <algm-LoadHKL>` reads hkl output that includes direction cosines.
- :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` has DetCal information sorted by detector numbers
- :ref:`StatisticsOfPeaksWorkspace <algm-StatisticsOfPeaksWorkspace>` has resolution shells in units of d-Spacing.
- SpaceGroup now has a method to check whether a specified unit cell is compatible with the symmetry operations of the group.


Engineering Diffraction
-----------------------

- The new algorithm :ref:`GSASIIRefineFitPeaks <algm-GSASIIRefineFitPeaks>`
  can be used to call the GSAS-II software repeatedly to refine lattice
  parameters (whole pattern refinement) and/or fit peaks.

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

- New algorithm :ref:`SaveNexusPD <algm-SaveNexusPD>` which creates a nexus file for use in GUDRUN and will hopefully be supported by Rietveld packages in the future.


Graphical user interface
########################

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

- Further improvements to Fitting tab, if for some reason the fitting
  fails, the focused workspace should still be plotted. This will
  enable user to select valid peaks and run Fit accordingly. User also
  now have an option to plot single peak fitting in separate workspace
  by using *Plot To Separate Window* button. Peak list can now also be
  cleared using the *Clear* button.

Imaging
-------

- The new algorithm `ImggAggregateWavelengths 
  <http://docs.mantidproject.org/v3.7.1/algorithms/ImggAggregateWavelengths-v1.html>`_
  aggregates stacks of images from wavelength dependent data.

- The algorithm `ImggTomographicReconstruction 
  <http://docs.mantidproject.org/v3.7.1/algorithms/ImggTomographicReconstruction-v1.html>`_ 
  has been introduced. This is a
  first experimental version that implements the Filtered
  Back-Projection (FBP) reconstruction method using the FBP
  implementation of the `TomoPy package
  <http://www.aps.anl.gov/tomopy/>`_.
- Images loaded as Mantid workspaces can now be saved into FITS files
  using the algorithm :ref:`SaveFITS <algm-SaveFITS>`.


Improvements in the tomographic reconstruction graphical user interface
#######################################################################

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
  algorithm `ImggAggregateWavelengths 
  <http://docs.mantidproject.org/v3.7.1/algorithms/ImggAggregateWavelengths-v1.html>`_.


Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
