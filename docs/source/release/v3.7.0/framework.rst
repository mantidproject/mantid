=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

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

Renamed
#######

- The following Vesuvio specific algorithms have been updated to have their name prefixed by Vesuvio:
    - :ref:`VesuvioCalculateGammaBackground <algm-VesuvioCalculateGammaBackground>` previously ``CalculateGammaBackground``
    - :ref:`VesuvioCalculateMS <algm-VesuvioCalculateMS>` previously ``CalculateMSVesuvio``
    - :ref:`VesuvioDiffractionReduction <algm-VesuvioDiffractionReduction>` previously ``EVSDiffractionReduction``

Improved
########

-  :ref:`EnggCalibrate <algm-EnggCalibrate>`
   has a new output property with the fitted parameters of the
   calibration sample peaks. It also logs more details about the peaks
   fitted.
-  :ref:`Integration <algm-Integration>`
   now correctly works for event data that has not been binned.
-  :ref:`FFT <algm-FFT>`
   now has an extra (optional) parameter, ``AcceptXRoundingErrors``. When
   set, this enables the algorithm to run even when the bin widths are
   slightly different. (An error is still produced for large
   deviations). By default, this is set to false, keeping the original
   behaviour.
   `#15325 <https://github.com/mantidproject/mantid/pull/15325>`_
-  :ref:`ConvertUnits <algm-ConvertUnits>`
   now works correctly for 'distribution' data in a :ref:`MatrixWorkspace <MatrixWorkspace>` in
   in-place mode (``InputWorkspace`` = ``OutputWorkspace``).
   `#15489 <https://github.com/mantidproject/mantid/pull/15489>`_
-  When plotting a workspace that had been normalized by bin widths, the y-axis unit label was incorrect.
   An appropriate labelling has now been implemented
  `#15398 <https://github.com/mantidproject/mantid/pull/15398>`_
-  :ref:`SumSpectra <algm-SumSpectra>` fixed broken scaling of bins for the `WeightedSum=true` case.
-  :ref:`LoadISISNexus <algm-LoadISISNexus>`now works correctly for data with non-contiguous detector IDs for either monitors or detectors. `#15562 <https://github.com/mantidproject/mantid/pull/15562>`_
-  A bug has been fixed in several algorithms where they would crash when given a :ref:`WorkspaceGroup <WorkspaceGroup>` as input (if run in the GUI). These algorithms are: `#15584 <https://github.com/mantidproject/mantid/pull/15584>`_
   - :ref:`AsymmetryCalc <algm-AsymmetryCalc>`
   - :ref:`CalMuonDetectorPhases <algm-CalMuonDetectorPhases>`
   - :ref:`ConvertToDistribution <algm-ConvertToDistribution>`
   - :ref:`ChangeTimeZero <algm-ChangeTimeZero>`
   - :ref:`FFT <algm-FFT>`
   - :ref:`MaxEnt <algm-MaxEnt>`
- :ref:`LoadNexusMonitors <algm-LoadNexusMonitors>`
  now allow user to choose to load either histogram monitor or event monitor only with 2 new
  properties (``LoadEventMonitor`` and ``LoadHistogramMonitor``).
  `#15667 <https://github.com/mantidproject/mantid/pull/15667>`_
- :ref:`CreateSimulationWorkspace <algm-CreateSimulationWorkspace>` now matches the IDF of the simulation workspace to the IDF of a reference workspace (either Nexus or Raw).
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` now correctly reads in event data that does not have a common x-axis. `#15746 <https://github.com/mantidproject/mantid/pull/15746>`
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` allows now to load logs from an entry other than the first. :ref:`LoadEventNexus <algm-LoadEventNexus>` now loads the correct logs when an *NXentry* is given
- :ref:`FFT <algm-FFT>`: added property *AutoShift* to enable automatic phase correction for workspaces not centred at zero.
- :ref:`SaveMD <algm-SaveMD>` now writes MDHisto signal arrays as compressed data.

Deprecated
##########

-  The `UserAlgorithms` package is no longer being shipped with the Windows packages.

MD Algorithms (VATES CLI)
#########################

-  The algorithm :ref:`SaveMDWorkspaceToVTK <algm-SaveMDWorkspaceToVTK>` is now available. It allows the
   user to save 3D MDHisto or 3D MDEvent workspaces as either a ``.vts`` or
   ``.vtu`` files. These file types can be loaded into a standalone version
   of ParaView.
-  PlotMD now plots points at bin centres for MDEventWorkspaces as well as MDHistoWorkspaces.
-  SliceMD now reports the correct number of events in the output workspace.
-  The size of densely populated, multidimensional MDEventWorkspace slices produced by SliceMD has been greatly reduced by using more sensible box splitting parameters.
-  MergeMD now does not add masked events to its output workspace.

Geometry
--------

The Instrument Definition File syntax has been extended to provide support for a new type of topologically regular, but geometrically irregular form of 2D detectors. This new type of detector available in the IDF is known as a Structured Detector. Information on how to use this new detector type can be found in the :ref:`IDF <InstrumentDefinitionFile>` documentation. 

Performance
-----------

- :ref:`ChangeBinOffset <algm-ChangeBinOffset>` should now run faster for a :ref:`MatrixWorkspace <MatrixWorkspace>` (not EventWorkspaces).
- Applying ParameterMaps to Detectors now about 30% faster. Algorithms that involve applying ParameterMaps will see performance improvements.
- This release saw the introduction of the StructuredDetector. This change has reduced load times via :ref:`LoadInstrument <algm-LoadInstrument>` from ~10minutes down to ~1second for the prospective ESS LOKI instrument. 

CurveFitting
------------

- Concept page for :ref:`Mantid Fitting <Fitting>` has been added.

Improved
########

Python
------

Python Algorithms
#################


Script Repository
-----------------

- A bug has been fixed that caused uploads to fail with some incorrectly configured proxy servers.

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
