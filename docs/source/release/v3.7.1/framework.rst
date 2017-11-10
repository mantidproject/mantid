=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:


Geometry
--------

- The Instrument Definition File syntax has been extended to provide support for a new type of topologically regular, but geometrically irregular form of 2D detectors. This new type of detector available in the IDF is known as a Structured Detector. Information on how to use this new detector type can be found in the :ref:`IDF <InstrumentDefinitionFile>` documentation.
- The XML shape definitions have been updated to understand a hollow cylinder as a primitive shape. See :ref:`HowToDefineGeometricShape` for more details.

Refactored `PeakShape` to better support arbitrary shapes.

Performance
-----------

- :ref:`ChangeBinOffset <algm-ChangeBinOffset>` should now run faster for a :ref:`MatrixWorkspace <MatrixWorkspace>` (not EventWorkspaces).
- Applying ParameterMaps to Detectors now about 30% faster. Algorithms that involve applying ParameterMaps will see performance improvements.
- This release saw the introduction of the StructuredDetector. This change has reduced load times via LoadInstrument from ~10minutes down to ~0.5seconds for the prospective ESS LOKI instrument.
  For more information on how to generate a StructuredDetector based instrument follow
  this `link <http://docs.mantidproject.org/nightly/concepts/InstrumentDefinitionFile.html#creating-structured-irregular-geometry-detectors>`_
- The destructors for ConvexPolygon and Quadrilateral objects are now faster, especially on Linux.

CurveFitting
------------

- Concept page for :ref:`Mantid Fitting <Fitting>` has been added.
- Concept page for :ref:`Comparing fit minimizers <FittingMinimizers>` has been added to provide information on the minimizers supported, and compare how well different minimizers do against different benchmark fitting problems
- In order to guarantee a complete overlap between resolution and signal in the region of interest, Function :ref:`Convolution <func-Convolution>` can switch between a fast FFT mode for data defined over a symmetric domain, and slower direct calculations for data defined over an asymmetric domain.

The work on benchmarking fitting has received funding from the Horizon 2020 Framework Programme of the European Union under the SINE2020 project Grant No 654000.

Script Repository
-----------------

- A bug has been fixed that caused uploads to fail with some incorrectly configured proxy servers.
- The default timeout has been increased from 5s to 30s to avoid hitting the timeout when tying to download some larger files.

Algorithms
----------

Properties
##########
-  String properties of algoithms are now trimmed of whitespace by default before being used by the algorithm.  So "  My filename   " will be trimmed to "My filename".

New
###

-  :ref:`AlignComponents <algm-AlignComponents>`
   This algorithm will take the calibration from the output of
   :ref:`GetDetOffsetsMultiPeaks <algm-GetDetOffsetsMultiPeaks>`, :ref:`CalibrateRectangularDetectors <algm-CalibrateRectangularDetectors>`, *et al* and
   minimizes the difference between the *DIFC* of the instrument and
   calibration by moving and rotating instrument components.
- :ref:`CorrectTOF <algm-CorrectTOF>` applies to the time-of-flight correction which considers the specified elastic peak position.
- :ref:`EnggFitDIFCFromPeaks <algm-AlignComponents>` fits GSAS calibration
  parameters (DIFA, DIFC, TZERO) from peaks fitted using
  :ref:`EnggFitPeaks <algm-EnggFitPeaks>`.
- :ref:`FindEPP <algm-FindEPP>` This algorithm performs Gaussian fit to find the elastic peak position.
  As a result, `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ with parameters of the fitted peaks is created.
- :ref:`GetIPTS <algm-GetIPTS>` Returns the IPTS directory of the specified ORNL run.
- :ref:`GSASIIRefineFitPeaks <algm-GSASIIRefineFitPeaks>` uses the GSAS-II
  software to refine lattice parameters (whole pattern refinement) and fit
- `ImggAggregateWavelengths <http://docs.mantidproject.org/v3.7.1/algorithms/ImggAggregateWavelengths-v1.html>`_ 
  aggregates stacks of images from wavelength dependent imaging into one or more output bands.
- `ImggTomographicReconstruction <http://docs.mantidproject.org/v3.7.1/algorithms/ImggTomographicReconstruction-v1.html>`_
  implements a method for 3D tomographic reconstruction from projection images.
- :ref:`SaveFITS <algm-SaveFITS>` saves images in FITS format.

Renamed
#######

- The following Vesuvio specific algorithms have been updated to have their name prefixed by Vesuvio:
    - :ref:`VesuvioCalculateGammaBackground <algm-VesuvioCalculateGammaBackground>` previously ``CalculateGammaBackground``
    - :ref:`VesuvioCalculateMS <algm-VesuvioCalculateMS>` previously ``CalculateMSVesuvio``
    - :ref:`VesuvioDiffractionReduction <algm-VesuvioDiffractionReduction>` previously ``EVSDiffractionReduction``

Improved
########

- :ref:`EnggCalibrate <algm-EnggCalibrate>` has a new output property
  with the fitted parameters of the calibration sample peaks. It also
  logs more details about the peaks fitted.
- :ref:`EnggFocus<algm-EnggFocus>`: added an option to mask out
  several ranges in ToF (instrument pulses), with default values set
  for ENGIN-X, and an option to normalize by proton charge (enabled by
  default).
-  :ref:`Integration <algm-Integration>`
   now correctly works for event data that has not been binned.
-  :ref:`FFT <algm-FFT>`
   now has an extra (optional) parameter, ``AcceptXRoundingErrors``. When
   set, this enables the algorithm to run even when the bin widths are
   slightly different. (An error is still produced for large
   deviations). By default, this is set to false, keeping the original
   behaviour.
-  :ref:`ConvertUnits <algm-ConvertUnits>`
   now works correctly for 'distribution' data in a :ref:`MatrixWorkspace <MatrixWorkspace>` in
   in-place mode (``InputWorkspace`` = ``OutputWorkspace``).
-  When plotting a workspace that had been normalized by bin widths, the y-axis unit label was incorrect.
   An appropriate labelling has now been implemented.
-  :ref:`SumSpectra <algm-SumSpectra>` fixed broken scaling of bins for the `WeightedSum=true` case.
-  :ref:`LoadISISNexus <algm-LoadISISNexus>` now works correctly for data with non-contiguous detector IDs for either monitors or detectors.
-  A bug has been fixed in several algorithms where they would crash when given a :ref:`WorkspaceGroup <WorkspaceGroup>` as input (if run in the GUI). These algorithms are:

   - :ref:`AsymmetryCalc <algm-AsymmetryCalc>`
   - :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`
   - :ref:`ConvertToDistribution <algm-ConvertToDistribution>`
   - :ref:`ChangeTimeZero <algm-ChangeTimeZero>`
   - :ref:`FFT <algm-FFT>`
   - :ref:`MaxEnt <algm-MaxEnt>`

- :ref:`LoadNexusMonitors <algm-LoadNexusMonitors>`
  now allow user to choose to load either histogram monitor or event monitor only with 2 new
  properties (``LoadEventMonitor`` and ``LoadHistogramMonitor``).
- :ref:`CreateSimulationWorkspace <algm-CreateSimulationWorkspace>` now matches the IDF of the simulation workspace to the IDF of a reference workspace (either Nexus or Raw).
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` now correctly reads in event data that does not have a common x-axis.
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` allows now to load logs from an entry other than the first. :ref:`LoadEventNexus <algm-LoadEventNexus>` now loads the correct logs when an *NXentry* is given
- :ref:`FFT <algm-FFT>`: added property *AutoShift* to enable automatic phase correction for workspaces not centred at zero.
- :ref:`SaveAscii <algm-SaveAscii>` now has a SpectrumMetaData property that allows for addition information to be displayed along with the SpectrumNumber. Currently the supported MetaData is SpectrumNumber, Q and Angle.
- :ref:`SaveMD <algm-SaveMD>` now writes MDHisto signal arrays as compressed data.
- :ref:`SetUncertainties <algm-SetUncertainties>` has two new modes, ``oneIfZero`` and ``sqrtOrOne``.
- :ref:`SetSampleMaterial <algm-SetSampleMaterial>` will now work out the number density from the chemical formula and mass density if these are given in the input. A user specified number density or if Z and the unit cell volume is given will override the value calculated from the chemical formula and mass density.
- :ref:`ComputeCalibrationCoefVan <algm-ComputeCalibrationCoefVan>`
  does not perform fit of data by itself, but requires an additional argument: ``EPPTable``. This should accelerate the data reduction workflow, because fitting results can be reused. Table with elastic peak positions can be created using the new :ref:`FindEPP <algm-FindEPP>` algorithm.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` now supports inelastic instruments. It relies on :ref:`ConvertUnits <algm-ConvertUnits>` having set the correct EMode.
- :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` has been fixed to normalize the data correctly. Previous versions contained a bug when removing the bin-width normalization that may have already been present on the input data


Deprecated
##########

-  The `UserAlgorithms` package is no longer being shipped with the Windows packages.

.. _R3.7 Vates CLI:

MD Algorithms (VATES CLI)
#########################

-  The algorithm :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` is now available. It allows the
   user to save 3D MDHisto or 3D MDEvent workspaces as either a ``.vts`` or
   ``.vtu`` files. These file types can be loaded into a standalone version
   of ParaView.
-  PlotMD now plots points at bin centres for MDEventWorkspaces as well as MDHistoWorkspaces.
-  SliceMD now reports the correct number of events in the output workspace.
-  The size of densely populated, multidimensional MDEventWorkspace slices produced by SliceMD has been greatly reduced by using more sensible box splitting parameters.
-  MD slicing algorithms now correctly detect units in input workspace and set units in output workspace as directed with the BasisVector properties.
-  Slicing algorithms (SliceMD and BinMD) do not add masked data to their output workspaces.
-  MergeMD now does not add masked events to its output workspace.
-  ConvertToMD, CreateMD and AccumulateMD now have the option to produce workspaces with a file-backend.
-  Dimension labelling in MD slicing algorithms is consistent with ConvertToMD.
-  The box structure of workspaces created with CutMD using NoPix=false now matches that specified by the PnBins properties. Additional box splitting is only allowed if MaxRecursionDepth is set to higher than its default of 1.
-  XorMD, OrMD and AndMD treat masked bins as zero.
-  A Gaussian smoothing option has been added to SmoothMD. Note, this currently only supports specifying widths for the smoothing function in units of pixels along the dimensions of the workspace.
-  LoadMD has an option to skip loading workspace history. This is useful for workspaces created form large number of files, treated separately.


Python
------

- It is now possible to use the unit.quickConversion(destinationUnit) functionality in python. If it is possible to convert one unit to another using a multiplication by a constant, this will return the factor and power required for the multiplication.

- The Atom kernel class, which stores the cross-sections, relative atomic masses, and other information for all elements and isotopes is now accessible from Python.

- The Material class has two new Python methods: ``chemicalFormula`` returns a tuple of Atom objects corresponding to the atoms in the compound, and their abundances; ``relativeMolecularMass`` returns the relative formular unit mass in atomic mass units.

- The plot() function of mantidplot.pyplot now supports empty marker (marker=None).

- V3D is now iterable in Python, which makes it possible to easily construct numpy arrays like this ``np.array(V3D(1,2, 3))``.

- Two new attributes available on all python algorithms ``startProgress`` and ``endProgress``. Added to an algorithm call, it will allow for passing control of the progress bar to child algorithms.


Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
