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
:ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` will now load instrument geometry from hdf5 `NeXus <https://www.nexusformat.org/>`_ format files. Files consistent with the standard following the introduction of `NXoff_geometry <http://download.nexusformat.org/sphinx/classes/base_classes/NXoff_geometry.html>`_ and `NXcylindrical_geometry <http://download.nexusformat.org/sphinx/classes/base_classes/NXcylindrical_geometry.html>`_ will be used to build the entire in-memory instrument geometry within Mantid. This IDF-free route is primarily envisioned for the ESS. This marks the completion of the first phase in the feasibility and rollout of support for the new format. Over coming releases we will be expanding our support for the NeXus geometry both across Loading and Saving algorithms. While dependent on the instrument, we are overall seeing significant improvements in instrument load times over loading from equivalent IDF based implementations.

Archive Searching / ONCat
-------------------------

- For HFIR instruments that write out raw files with run numbers, we have enabled functionality that allows for the searching of file locations by making calls to ONCat.  To use this, make sure that the "Search Data Archive" option is checked in your "Manage User Directories" settings.  The ``FileFinder`` and algorithms such as :ref:`Load <algm-Load>`  will then accept inputs such as "``HB2C_143210``".
- In the short term, for SNS instruments, the default "out of the box" behaviour will be to continue using ICAT for archive searching.  This can be manually overridden by changing ``<archiveSearch plugin="SNSDataSearch" />`` to ``<archiveSearch plugin="ORNLDataSearch" />`` in your ``Facilities.xml`` file.  Existing functionality should be almost completely unaffected, except that you may find that archive searching is a little quicker and slightly more robust.  ``ORNLDataSearch`` will become the default option at SNS in a future version of Mantid.

Stability
---------


Algorithms
----------

New Algorithms
##############

- :ref:`CalculateDynamicRange <algm-CalculateDynamicRange>` will calculate the Q range of a SANS workspace.
- :ref:`MatchSpectra <algm-MatchSpectra>` is an algorithm that calculates factors to match all spectra to a reference spectrum.
- :ref:`MaskBinsIf <algm-MaskBinsIf>` is an algorithm to mask bins according to criteria specified as a muparser expression.
- :ref:`MaskNonOverlappingBins <algm-MaskNonOverlappingBins>` masks the bins that do not overlap with another workspace.

Improvements
############

- :ref:`AppendSpectra <algm-AppendSpectra>` can append now multiple times the same event workspace.
- :ref:`CropToComponent <algm-CropToComponent>` now supports also scanning workspaces.
- :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>` will produce histogram data, and will not split the counts between bins by default.
- :ref:`SumSpectra <algm-SumSpectra>` has an additional option, ``MultiplyBySpectra``, which controls whether or not the output spectra are multiplied by the number of bins. This property should be set to ``False`` for summing spectra as PDFgetN does.
- :ref:`Live Data <algm-StartLiveData>` for events with ``PreserveEvents=True`` now produces workspaces that have bin boundaries which encompass the total x-range (TOF) for all events across all spectra if the data was not binned during the process step.
- :ref:`RebinToWorkspace <algm-RebinToWorkspace>` now checks if the ``WorkspaceToRebin`` and ``WorkspaceToMatch`` already have the same binning. Added support for ragged workspaces.
- :ref:`GroupWorkspaces <algm-GroupWorkspaces>` supports glob patterns for matching workspaces in the ADS.
- :ref:`LoadSampleShape <algm-LoadSampleShape-v1>` now supports loading from binary .stl files.
- :ref:`MaskDetectorsIf <algm-MaskDetectorsIf>` now supports masking a workspace in addition to writing the masking information to a calfile.
- :ref:`LoadSampleShape <algm-LoadSampleShape-v1>` now supports loading from binary .stl files.
- :ref:`LoadNexusLogs <algm-LoadNexusLogs-v1>` now will load files that have 1D arrays for each time value in the logs, but will not load this data.
- :ref:`GroupDetectors <algm-GroupDetectors>` now takes masked bins correctly into account when processing histogram workspaces.

Bugfixes
########

- :ref:`SaveGDA <algm-SaveGDA>` Now takes a parameter of OutputFilename instead of Filename to better match with similar algorithms.
- Bugfix in :ref:`ConvertToMatrixWorkspace <algm-ConvertToMatrixWorkspace>` with ``Workspace2D`` as the ``InputWorkspace`` not being cloned to the ``OutputWorkspace``. Added support for ragged workspaces.
- :ref:`SolidAngle <algm-SolidAngle-v1>` Now properly accounts for a given StartWorkspaceIndex.
- :ref:`FilterEvents <algm-FilterEvents-v1>` output workspaces now contain the goniometer.
- Fixed an issue where if a workspace's history wouldn't update for some algorithms
- Fixed a ``std::bad_cast`` error in :ref:`algm-LoadLiveData` when the data size changes.
- :ref:`Fit <algm-Fit>` now applies the ties in correct order independently on the order they are set. If any circular dependencies are found Fit will give an error.
- Fixed a rare bug in :ref:`MaskDetectors <algm-MaskDetectors>` where a workspace could become invalidaded in Python if it was a ``MaskWorkspace``.
- Fixed a crash in :ref:`MaskDetectors <algm-MaskDetectors>` when a non-existent component was given in ``ComponentList``.
- History for algorithms that took groups sometimes would get incorrect history causing history to be incomplete, so now full group history is saved for all items belonging to the group.
- Algorithm :ref:`algm-Stitch1D-v3` takes correct user input of `EndOverlap` values into account.

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

Improvements
############

- :ref:`ChudleyElliot <func-ChudleyElliot>` includes hbar in the definition
- :ref:`Functions <FitFunctionsInPython>` may now have their constraint penalties for fitting set in python using ``function.setConstraintPenaltyFactor("parameterName", double)``.
- :py:obj:`mantid.kernel.Logger` now handles unicode in python2


Bugfixes
########


:ref:`Release 3.14.0 <v3.14.0>`
