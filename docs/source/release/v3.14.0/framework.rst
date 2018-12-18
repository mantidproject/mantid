=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Logging
-------

- We have changed the logging in Mantid to stop writing the high level version of the log to a file.  This had been causing numerous problems including inconsistent behaviour with multiple instances of Mantid, performance problems when logging at detailed levels, and excessive network usage in some scenarios.  This does not change the rest of the logging that you see in the message display in Mantidplot or the console window. A warning message will appear if configuration for the removed components of logging is found.

- Associated with this we have also simplified the python methods used to control logging.

.. code-block:: python

   # The two methods
   ConfigService.SetConsoleLogLevel(int)
   ConfigService.SetFileLogLevel(int)

   # Have been replaced by
   ConfigService.SetLogLevel(int)

Nexus Geometry Loading
----------------------
:ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` will now load instrument geometry from hdf5 `NeXus <https://www.nexusformat.org/>`_ format files. Files consistent with the standard following the introduction of `NXoff_geometry <http://download.nexusformat.org/sphinx/classes/base_classes/NXoff_geometry.html>`_ and `NXcylindrical_geometry <http://download.nexusformat.org/sphinx/classes/base_classes/NXcylindrical_geometry.html>`_ will be used to build the entire in-memory instrument geometry within Mantid. This IDF-free route is primarily envisioned for the ESS. While dependent on the instrument, we are overall seeing significant improvements in instrument load times over loading from equivalent IDF based implementations. :ref:`LoadInstrument <algm-LoadInstrument>` also supports the nexus geometry format in the same was as LoadEmptyInstrument.

The changes above have also been encorporated directly into :ref:`LoadEventNexus <algm-LoadEventNexus>`, so it is possible to store data and geometry together for the first time in a NeXus compliant format for Mantid. These changes have made it possible to load experimental ESS event data files directly into Mantid.


Archive Searching
-----------------

SNS / ONCat
###########

- SNS file searching has been moved to `ONCAT <https://oncat.ornl.gov/>`_
- For HFIR instruments that write out raw files with run numbers, we have enabled functionality that allows for the searching of file locations by making calls to ONCat.  To use this, make sure that the "Search Data Archive" option is checked in your "Manage User Directories" settings.  The ``FileFinder`` and algorithms such as :ref:`Load <algm-Load>`  will then accept inputs such as "``HB2C_143210``".

ISIS / ICat
###########

- The path returned by ICat starting ``\\isis\inst$`` is now overridden by the fully-qualified path starting ``\\isis.cclrc.ac.uk\inst$`` on Windows machines. This makes accessing the archive more reliable over the VPN.
- On Linux and Mac machines, it is overridden by a path starting ``/archive``.
- On all machines, you can override this locally by setting ``icatDownload.mountPoint=<my_path>`` in your ``Mantid.user.properties`` file.

Stability
---------


Algorithms
----------

New Algorithms
##############

- :ref:`DeadTimeCorrection <algm-DeadTimeCorrection>` will correct for the detector dead time.
- :ref:`CalculateDynamicRange <algm-CalculateDynamicRange>` will calculate the Q range of a SANS workspace.
- :ref:`MatchSpectra <algm-MatchSpectra>` is an algorithm that calculates factors to match all spectra to a reference spectrum.
- :ref:`MaskBinsIf <algm-MaskBinsIf>` is an algorithm to mask bins according to criteria specified as a muparser expression.
- :ref:`MaskNonOverlappingBins <algm-MaskNonOverlappingBins>` masks the bins that do not overlap with another workspace.
- :ref:`ParallaxCorrection <algm-ParallaxCorrection>` will perform a geometric correction for the so-called parallax effect in tube based SANS detectors.

Improvements
############

- :ref:`AppendSpectra <algm-AppendSpectra>` can append now multiple times the same event workspace.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` can merge sample logs according to the parameter file independently from :ref:`MergeRuns <algm-MergeRuns>`. All parameter names must have the prefix ``conjoin_`` appended by the corresponding default parameter names (which are used by :ref:`MergeRuns <algm-MergeRuns>`).
- :ref:`CropToComponent <algm-CropToComponent>` now supports also scanning workspaces.
- :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>` will produce histogram data, and will not split the counts between bins by default.
- :ref:`SumSpectra <algm-SumSpectra>` has an additional option, ``MultiplyBySpectra``, which controls whether or not the output spectra are multiplied by the number of bins. This property should be set to ``False`` for summing spectra as PDFgetN does.
- :ref:`Live Data <algm-StartLiveData>` for events with ``PreserveEvents=True`` now produces workspaces that have bin boundaries which encompass the total x-range (TOF) for all events across all spectra if the data was not binned during the process step.
- :ref:`RebinToWorkspace <algm-RebinToWorkspace>` now checks if the ``WorkspaceToRebin`` and ``WorkspaceToMatch`` already have the same binning. Added support for ragged workspaces.
- :ref:`GroupWorkspaces <algm-GroupWorkspaces>` supports glob patterns for matching workspaces in the ADS.
- :ref:`LoadSampleShape <algm-LoadSampleShape-v1>` now supports loading from binary .stl files.
- :ref:`MaskDetectorsIf <algm-MaskDetectorsIf>` now supports masking a workspace in addition to writing the masking information to a calfile.
- :ref:`ApplyDetectorScanEffCorr <algm-ApplyDetectorScanEffCorr>` will properly propagate the masked bins in the calibration map to the output workspace.
- :ref:`LoadSampleShape <algm-LoadSampleShape-v1>` now supports loading from binary .stl files.
- :ref:`LoadNexusLogs <algm-LoadNexusLogs-v1>` now will load files that have 1D arrays for each time value in the logs, but will not load this data.
- :ref:`GroupDetectors <algm-GroupDetectors>` now takes masked bins correctly into account when processing histogram workspaces.
- :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` and :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` can now save and load a ``MaskWorkspace``.
- :ref:`FitPeaks <algm-FitPeaks>` can output parameters' uncertainty (fitting error) in an optional workspace.

Bugfixes
########

- :ref:`SaveGDA <algm-SaveGDA>` Now takes a parameter of OutputFilename instead of Filename to better match with similar algorithms.
- Bugfix in :ref:`ConvertToMatrixWorkspace <algm-ConvertToMatrixWorkspace>` with ``Workspace2D`` as the ``InputWorkspace`` not being cloned to the ``OutputWorkspace``. Added support for ragged workspaces.
- :ref:`SolidAngle <algm-SolidAngle-v1>` Now properly accounts for a given StartWorkspaceIndex.
- :ref:`FilterEvents <algm-FilterEvents-v1>` output workspaces now contain the goniometer.
- Fixed an issue where if a workspace's history wouldn't update for some algorithms
- Fixed a ``std::bad_cast`` error in :ref:`algm-LoadLiveData` when the data size changes.
- :ref:`Fit <algm-Fit>` now applies the ties in correct order independently on the order they are set. If any circular dependencies are found Fit will give an error.
- Fixed a rare bug in :ref:`MaskDetectors <algm-MaskDetectors>` where a workspace could become invalidated in Python if it was a ``MaskWorkspace``.
- Fixed a crash in :ref:`MaskDetectors <algm-MaskDetectors>` when a non-existent component was given in ``ComponentList``.
- The output workspace now keeps the units of the input workspace for all sample log entries of algorithms :ref:`MergeRuns <algm-MergeRuns>` and :ref:`ConjoinXRuns <algm-ConjoinXRuns>`.
- History for algorithms that took groups sometimes would get incorrect history causing history to be incomplete, so now full group history is saved for all items belonging to the group.
- Fixed a bug in `SetGoniometer <algm-SetGoniometer>` where it would use the mean log value rather than the time series average value for goniometer angles.
- `ConvertToMD <algm-ConvertToMD>` now uses the time-average value for logs when using them as ``OtherDimensions``
- The input validator is fixed in :ref:`MostLikelyMean <algm-MostLikelyMean>` avoiding a segmentation fault.
- Fixed a bug in :ref:`ExtractSpectra <algm-ExtractSpectra>` which was causing a wrong last value in the output's vertical axis if the axis type was ``BinEdgeAxis``.

Python
------

New
###

- All python methods accepting basic strings now also accept unicode strings.
- New python validator type: :class:`~mantid.geometry.OrientedLattice` checks whether a workspace has an oriented lattice object attached.
- The windows python bundle now includes numpy=1.15.4, scipy=1.1.0, matplotlib=2.2.3, pip=18.1
- We have been making major performance improvements to geometry access in Mantid over the last few releases. We are now exposing these features via Python to give our users direct access to the same benefits as part of their scripts. The newly exposed objects are now available via workspaces and include:

 * :class:`mantid.geometry.ComponentInfo`
 * :class:`mantid.geometry.DetectorInfo`
 * :class:`mantid.api.SpectrumInfo`

- :class:`mantid.geometry.ComponentInfo` is exposed to allow the user to access geometric information about the components which are part of a beamline.
- :class:`mantid.geometry.DetectorInfo` offers the user the ability to access geometric information about the detector(s) which are part of a beamline. ``DetectorInfo`` has also been given an iterator to allow users to write more Pythonic loops rather than normal index based loops.
- :class:`mantid.api.SpectrumInfo` allows the user to access information about the spectra being used in a beamline. ``SpectrumInfo`` has also been given an iterator to allow users to write more Pythonic loops rather than normal index based loops. In addition to this ``SpectrumDefinition`` objects can also be accessed via a :class:`mantid.api.SpectrumInfo` object. The ``SpectrumDefinition`` object can be used to obtain information about the spectrum to detector mapping and provides a definition of what a spectrum comprises, i.e. indices of all detectors that contribute to the data stored in the spectrum.
- Added new :ref:`unit <Unit Factory>` called ``Temperature`` which has units of Kelvin.
- Importing ``mantid`` no longer initializes the ``FrameworkManager``. This allows separate classes to be imported without requiring a long delay in waiting for the framework to start. Amongst other things this allows the application name to be set correctly:

.. code-block:: python

   from mantid import FrameworkManager, UsageService
   UsageService.setApplicationName('myapp')
   FrameworkManager.Instance()


Improvements
############

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition
- :ref:`Functions <FitFunctionsInPython>` may now have their constraint penalties for fitting set in python using ``function.setConstraintPenaltyFactor("parameterName", double)``.
- :py:obj:`mantid.kernel.Logger` now handles unicode in python2


Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`
