=================
Framework Changes
=================

Algorithms
----------

New features
############
- (`#41439 <https://github.com/mantidproject/mantid/pull/41439>`_) The :ref:`algm-Fit` algorithm now allows setting step sizes to customize the perturbation applied to each parameter value while fitting. Setting the ``StepSizeMethod`` to ``Custom`` will allow the step sizes list to be passed using the ``CustomStepSizes`` parameter.
- (`#41714 <https://github.com/mantidproject/mantid/pull/41714>`_) Algorithm :ref:`algm-ConvertToMD` with option `UseLogTimes` can use the time series logs at event times as coordinates for extra dimensions when log names are added in `OtherDimensions` property.
- (`#41402 <https://github.com/mantidproject/mantid/pull/41402>`_) Algorithm :ref:`algm-ConvertToMD` now accepts an option, `UseLogTimes`, to use log times of neutron events in coordinate calculations instead of their average.
- (`#41427 <https://github.com/mantidproject/mantid/pull/41427>`_) :ref:`FileFinder <mantid.api.FileFinderImpl>` and :py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>` now resolve multi-run hint strings (ranges and lists) in a single batched archive lookup for HFIR and SNS data, replacing the previous per-run network round trips to ONCat.
- (`#41457 <https://github.com/mantidproject/mantid/pull/41457>`_) A new property ``OutputWorkspaceSuffixes`` has been added to the :ref:`algm-Stitch1DMany` algorithm, which allows users to specify custom suffixes for the child workspaces in the output group when stitching workspace groups.
- (`#41510 <https://github.com/mantidproject/mantid/pull/41510>`_) :ref:`algm-HFIRPowderReduction` now attempts to autopopulate the sample absorption parameters if correct data is found in sample run files
- (`#41601 <https://github.com/mantidproject/mantid/pull/41601>`_) :ref:`Stitch1D <algm-Stitch1D>` and :ref:`Stitch1DMany <algm-Stitch1DMany>` now have a ``UseValidDataOnly`` option which, when ``true``, ignores the contribution of invalid signal values to overlap bins when another workspace has valid data in the same bin. When ``false`` (default) previous behaviour is maintained, where presence of invalid data in an overlap bin from either workspace results in the same invalid value in the output workspace.
- (`#41778 <https://github.com/mantidproject/mantid/pull/41778>`_) The :ref:`algm-VesuvioTransmission` algorithm has been added which evaluates the transmission spectrum on the ``VESUVIO`` spectrometer for measured sample and empty run numbers.
- (`#41618 <https://github.com/mantidproject/mantid/pull/41618>`_) :ref:`algm-SumSpectra` algorithm has been optimized for better performance.
- (`#41757 <https://github.com/mantidproject/mantid/pull/41757>`_) :ref:`algm-HFIRPowderReduction` now has an ``IDFFilename`` property to optionally override the instrument geometry from the sample file, and loads MIDAS data with :ref:`algm-LoadEventAsWorkspace2D`.
- (`#41802 <https://github.com/mantidproject/mantid/pull/41802>`_) :ref:`algm-MDNormDirectSC` can now handle MDEvent workspaces created by :ref:`algm-ConvertToMD` with the ``UseLogTimes`` property.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) :ref:`algm-FitPeaks` now has ``StrictConvergence`` flag, which decides whether fits which have finished due to the parameter step size passing below the stopping threshold should be considered as successful - default is ``True`` to match previous behaviour.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) :ref:`algm-FitPeaks` now accepts ``Unweighted least squares`` cost function.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) :ref:`algm-FitPeaks` now has ``PositionToleranceMode`` input which controls ``PositionTolerance`` behaviour. Default is ``Check`` which maintains previous behaviour of the positional tolerance being checked after the fit and discarding any fits outside tolerance. This parameter enables ``Constrain`` mode, which will instead apply the tolerances as constraints on the fit. ``Constrain`` mode requires ``ConstrainPeakPositions`` be disabled.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) :ref:`algm-FitPeaks` now has ``CalculateUnconstrainedErrors`` which controls whether the errors for fits should be recomputed after the fit, without the constraints applied. Default is ``False``.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) :ref:`algm-FitPeaks` now has ``PositionToleranceFractional`` which controls whether the ``PositionTolerance`` should be treated as a fraction of the peak window (``True``) or as an absolute value (``False``, default).
- (`#41853 <https://github.com/mantidproject/mantid/pull/41853>`_) A new algorithm :ref:`algm-SaveMDToAscii` has been added to save an :py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>` to a plain ASCII file, with configurable exclusion of integrated dimensions, normalization, column separator and numeric precision.
- (`#41867 <https://github.com/mantidproject/mantid/pull/41867>`_) :ref:`algm-EstimatePeakIntensities` allows parallel, fit-free estimation of peak intensities within specified windows, using a skew-seeded background estimation.
- (`#41892 <https://github.com/mantidproject/mantid/pull/41892>`_) Parallelisation added to :ref:`algm-ConvertToMD` and improved in :ref:`algm-MDNormDirectSC`.
- (`#42028 <https://github.com/mantidproject/mantid/pull/42028>`_) :ref:`RenameWorkspace <algm-RenameWorkspace>` can now assign a name to a workspace object that is not already in the
  Analysis Data Service (ADS). In this case, the workspace will be added to the ADS.
- (`#42124 <https://github.com/mantidproject/mantid/pull/42124>`_) Added optional duration- and monitor-count normalization to
  :ref:`algm-HFIRGoniometerIndependentBackground`. Backgrounds can now be calculated from
  exposure-normalized rotations and either retained in normalized units or restored to the
  per-rotation exposure scale. WAND (HB2C) and DEMAND (HB3A) sample-log naming conventions
  are supported.
- (`#42124 <https://github.com/mantidproject/mantid/pull/42124>`_) :ref:`algm-HFIRGoniometerIndependentBackground` no longer accepts a negative ``BackgroundLevel``.
  The property is a percentile and must be between 0 and 100.
- (`#42126 <https://github.com/mantidproject/mantid/pull/42126>`_) :ref:`MDNorm <algm-MDNorm>` now supports continuous rotation data (generated by ConvertToMD with ``useLogTimes``).

Bugfixes
############
- (`#41427 <https://github.com/mantidproject/mantid/pull/41427>`_) :ref:`FileFinder <mantid.api.FileFinderImpl>` now recognises compound file extensions (e.g. ``.nxs.h5``) listed in ``Facilities.xml`` when expanding multi-run hints, so ranges such as ``CNCS100:105.nxs.h5`` resolve correctly instead of being misinterpreted as a stem ending in ``.nxs`` plus an ``.h5`` extension. As a consequence, a hint that explicitly specifies one extension (e.g. ``BSS_24234_event.nxs`` or ``SXD30904.raw.md5``) no longer silently resolves to a file with a different extension; specify the run number without an extension to let the facility's preferred extensions apply.
- (`#41749 <https://github.com/mantidproject/mantid/pull/41749>`_) :ref:`algm-SaveDaveGrp` algorithm now optionally converts the spectrum numbers into ``Q`` values using :ref:`algm-GetQsInQENSData` when creating the ``.grp`` file.
- (`#41559 <https://github.com/mantidproject/mantid/pull/41559>`_) In ``BeamProfileFactory`` the auto-generated beam profile dimensions are now correct when the sample bounding box is offset from the origin. The beam half-extent is now taken from the furthest sample face on either side, so the beam remains valid for samples that are translated or rotated into a negative half-space.
- (`#41559 <https://github.com/mantidproject/mantid/pull/41559>`_) In :ref:`EstimateScatteringVolumeCentreOfMass <algm-EstimateScatteringVolumeCentreOfMass>` the shape object now accounts for the goniometer set on the workspace. Previously this was not the case which gave erroneous values in non-rotationally symmetric situations.
- (`#41631 <https://github.com/mantidproject/mantid/pull/41631>`_) `:ref: CreateDetectorTable <algm-CreateDetectorTable>` has received some performance improvements that should speed up the algorithm significantly for large workspaces with many spectra. This algorithm is used in the new Instrument View when opening, so this should improve the performance of that interface as well.
- (`#41707 <https://github.com/mantidproject/mantid/pull/41707>`_) :ref:`ExtractSpectra <algm-ExtractSpectra>` no longer gives an error when the argument ``DetectorList`` is used on focused workspaces where a workspace index maps to several detector IDs.
- (`#41806 <https://github.com/mantidproject/mantid/pull/41806>`_) In :ref:`algm-FitPeaks`, the ``PeakParameterValueTable`` input option now reads and applies row-per-spectra parameter starting values. Previously this functionality was declared but was not applied.
- (`#41883 <https://github.com/mantidproject/mantid/pull/41883>`_) :ref:`algm-CopySample` no longer causes a crash when called with ``CopyShape = False``, ``CopyMaterial = True`` and an input workspace that is subsequently overwritten.
- (`#41886 <https://github.com/mantidproject/mantid/pull/41886>`_) Algorithm `SaveYDA` now correctly distinguishes between spectra and bin center axes when saving data.
- (`#42075 <https://github.com/mantidproject/mantid/pull/42075>`_) Unreadable instrument geometry cache is now deleted, and the cache gets recreated to prevent a load failure
- (`#42115 <https://github.com/mantidproject/mantid/pull/42115>`_) In :ref:`algm-HFIRPowderReduction`, the ``OutputWorskspace`` field now has a default based on the IPTS and run number. ``OutputDirectory`` now browses to a directory and user can manually set the file name by typing it in the box.
- (`#42134 <https://github.com/mantidproject/mantid/pull/42134>`_) Removed ``MultipleScattering`` checkbox from :ref:`algm-CylinderAbsorptionCW`, and renamed ``AttenuationXSection`` to ``AbsorptionXSection``
- (`#42207 <https://github.com/mantidproject/mantid/pull/42207>`_) :ref:`algm-SavePlot1D` no longer fails when run with plotly 7.0 or later. The ``plotly`` and ``plotly-full`` output types used plotly interfaces that were deprecated and then removed.
- (`#42256 <https://github.com/mantidproject/mantid/pull/42256>`_) :ref:`algm-BinaryOperateMasks` no longer throws a spurious validation error when acting upon input workspaces with non-default detector mapping.
-  Loading an instrument through ``InstrumentFileFinder`` now consistently selects parameter files matching the search string when multiple candidates have the same validity date. This prevents parameters from being loaded from less suitable files.

Deprecated
############
- (`#41906 <https://github.com/mantidproject/mantid/pull/41906>`_) The ``DPDFreduction`` algorithm has been deprecated and has no direct replacement.

Removed
############
- (`#41830 <https://github.com/mantidproject/mantid/pull/41830>`_) Removed SofQWCentre and SofQWPolygon (deprecated since release 6.12.0).
- (`#41781 <https://github.com/mantidproject/mantid/pull/41781>`_) Removed the deprecated BayesQuasi and BayesStretch algorithms, which were based on the ``quasielasticbayes`` library. Users should now use the new ``quickbayes`` replacements, :ref:`algm-BayesQuasi2` and :ref:`algm-BayesStretch2`, respectively.

Fit Functions
-------------

New features
############
- (`#41422 <https://github.com/mantidproject/mantid/pull/41422>`_) Poisson deviance is now a cost function for the Levenberg-Marquardt minimizer in fitting.

Bugfixes
############
- (`#41380 <https://github.com/mantidproject/mantid/pull/41380>`_) In an :ref:`InstrumentParameterFile`, defined fitting parameters no longer erroneously overwrite if they have the same name but for different functions (e.g. ``Bk2BkExpConvPV:Gamma`` and ``IkedaCarpenterPV:Gamma``).

Deprecated
############

Removed
############


Data Objects
------------

New features
############
- (`#41719 <https://github.com/mantidproject/mantid/pull/41719>`_) :ref:`SampleEnvironment` is now saved in the Nexus File
- (`#41901 <https://github.com/mantidproject/mantid/pull/41901>`_) `getInstrumentName()` can be used on workspaces or `ExperimentInfo` objects to get the name of the underlying instrument.  Use this instead of `getInstrument().getName()`.
- (`#42206 <https://github.com/mantidproject/mantid/pull/42206>`_) InstrumentMetadata is now exposed on workspaces and `ExperimentInfo` objects through `instrumentValidFromDate()`, `instrumentValidToDate()`, `instrumentFilename()`, `instrumentXmlText()`, `instrumentDefaultView()` and `instrumentDefaultAxis()`
-  Instrument parameters are now held in a new Instrument 2.0 ``ParameterInfo`` store and reached in C++ through ``ComponentInfo``, with no change to parameter values or to the Python instrument API. ``IComponent::getParameterNamesByComponent()`` and direct ``ParameterMap`` iteration have been removed; see :ref:`InstrumentAccessLayers` for the replacements.

Bugfixes
############
- (`#41794 <https://github.com/mantidproject/mantid/pull/41794>`_) Fixed a boundary-condition bug in ``MDGridBox`` splitting where events exactly on a child box's upper boundary could be dropped during serial or threaded splitting.


Live Data
---------

New features
############
- (`#42089 <https://github.com/mantidproject/mantid/pull/42089>`_) The live-data algorithms gain an optional ``Facility`` property, naming the facility that
  ``Instrument`` is resolved against. The Mantid default facility is used when it is not set. An
  instrument name is only meaningful with respect to a facility, so the two are now validated together,
  and a mismatch is reported against the offending property rather than raising an exception.

Bugfixes
############
- (`#41606 <https://github.com/mantidproject/mantid/pull/41606>`_) The ``SNSLiveEventDataListener`` state machine is refactored to simplify its structure, remove race conditions,
  and make all state transitions explicit.  These changes primarily add to the
  ``ILiveListener`` interface, deprecating its ``runStatus`` method,
  but otherwise leave most ``ILiveListener`` implementations largely unchanged.
- (`#41722 <https://github.com/mantidproject/mantid/pull/41722>`_) The socket-read poll loop of the ``SNSLiveEventDataListener`` is refactored to provide clean detection
  of a peer disconnect, and to improve its responsiveness. These changes are expected to fix several cases
  where the live-data system hung up in response to changes in the ADARA-server state.
- (`#42089 <https://github.com/mantidproject/mantid/pull/42089>`_) The live-data algorithms, such as :ref:`StartLiveData <algm-StartLiveData>`, previously could not use
  an instrument outside the default facility at all, since the allowed values for ``Instrument`` were
  built from the default facility alone. Naming the new ``Facility`` property now selects the facility
  that ``Instrument`` is resolved against.
- (`#42089 <https://github.com/mantidproject/mantid/pull/42089>`_) Resolving ``Instrument`` no longer falls back to searching every other facility once the resolved
  facility has been tried. That fallback made instrument names shared between facilities ambiguous, and
  worked against the purpose of having a default facility. Scripts that relied on an instrument being
  found outside the default facility must now name its ``Facility``.


Python
------

New features
############
- (`#41559 <https://github.com/mantidproject/mantid/pull/41559>`_) ``MeshObject`` now has ``getBoundingBox`` exposed to python.
- (`#41890 <https://github.com/mantidproject/mantid/pull/41890>`_) :py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>` now provides ``setNumEventsArray`` to overwrite the number-of-events array from a numpy array, mirroring the existing ``setSignalArray`` and ``setErrorSquaredArray`` methods. Manual updates through these array setters clear any link to the original ``MDEventWorkspace`` so downstream rebinning or slicing cannot bypass the manually-set values.
- (`#41922 <https://github.com/mantidproject/mantid/pull/41922>`_) The dimensions of an :class:`~mantid.api.IMDHistoWorkspace` can now have their ``name`` and ``units`` edited from
  Python via ``getDimension(i).setName(...)`` and ``getDimension(i).setUnits(...)``. Editing units is supported for
  HKL and general frames; an HKL dimension accepts ``r.l.u.`` or an inverse-Angstrom-style label such as
  ``in 2.5 A^-1``. This is useful for relabelling the dimensions produced by ``MDNorm`` for unusual projections.
- (`#42012 <https://github.com/mantidproject/mantid/pull/42012>`_) Added ``mantid.api.MinimizerStatus``, exposing the status strings a function minimizer reports through a fit's ``OutputStatus`` and an ``isConverged`` test for them, so Python callers no longer have to hard code the wording. By default the tolerance-limited stopping conditions count as converged alongside ``success``; pass ``strict=True`` for an exact match.
- (`#42028 <https://github.com/mantidproject/mantid/pull/42028>`_) Output workspace properties of the generic type ``Workspace`` now return a ``WorkspaceGroup`` object after an
  algorithm is automatically dispatched over an input workspace group. Previously, the value component of the
  property would be set to equal the ``WorkspaceGroup`` name but the workspace component would be null.
- (`#42205 <https://github.com/mantidproject/mantid/pull/42205>`_) Grid spacing, pixel counts and detector-ID numbering for rectangular/grid arrays can now be accessed from :class:`~mantid.geometry.ComponentInfo`, using the ``pixelGrid*`` methods such as ``pixelGridNX`` and ``pixelGridXStep``. Whether a component has such a grid can be tested with ``isGridDetector``.
-  Instrument parameters can now be read and written from :class:`~mantid.geometry.ComponentInfo` in Python, with ``hasParameter``, ``getNumberParameter``, ``addDouble`` and friends, replacing the equivalent methods on ``Instrument`` and its components. The optional ``index`` argument defaults to the instrument itself, as in ``component_info.getNumberParameter("x-pixel-size")``.
- (`#41739 <https://github.com/mantidproject/mantid/pull/41739>`_) ``Instrument`` now has ``getXmlText`` exposed to python.

Bugfixes
############
- (`#41876 <https://github.com/mantidproject/mantid/pull/41876>`_) Fixed a crash where a ``Peak`` retrieved from a ``PeaksWorkspace`` in Python could reference deleted memory after the owning workspace is deleted. Peaks now share ownership of their workspace, keeping it alive for as long as the peak is in use.

Deprecated
############
- (`#41756 <https://github.com/mantidproject/mantid/pull/41756>`_) :class:`~mantid.api.MatrixWorkspace` now exposes the preferred ``Histogram`` data accessors to python: ``x()``, ``y()``, ``e()``, ``dx()`` (read-only numpy wrappers) and ``mutableX()``, ``mutableY()``, ``mutableE()``, ``mutableDx()`` (writable numpy wrappers). These replace the legacy ``readX()``/``readY()``/``readE()``/``readDx()`` and ``dataX()``/``dataY()``/``dataE()``/``dataDx()`` methods, which are now deprecated and emit a ``DeprecationWarning`` naming the replacement, but continue to work and return the same data. See the :ref:`HistogramData` concept page for more information on the ``Histogram`` data model.
- (`#41756 <https://github.com/mantidproject/mantid/pull/41756>`_) The deprecation warnings above are emitted via `PyErr_Warn(PyExc_DeprecationWarning, ...) <https://docs.python.org/3/c-api/exceptions.html#c.PyExc_DeprecationWarning>`__, equivalent to ``warnings.warn(message, DeprecationWarning)``, and follow Python's standard warnings filtering: by default they are printed only for deprecated calls made directly from a user script or interactive session (``__main__``), once per call site, while calls from inside imported modules are silent. Run Python with ``-W default`` (or use ``warnings.simplefilter("default")``) to surface every occurrence, or ``error`` in place of ``default`` to raise them as exceptions when hunting call sites in tests or CI.
- (`#41756 <https://github.com/mantidproject/mantid/pull/41756>`_) ``dx()``/``mutableDx()`` behave the same way ``readDx()``/``dataDx()`` always have: if no Dx (x-error) data has been set on the spectrum, it is first initialized to zeros, so these methods are always safe to call without first checking :meth:`~mantid.api.MatrixWorkspace.hasDx`.
- (`#41768 <https://github.com/mantidproject/mantid/pull/41768>`_) ``ComponentInfo`` now exposes ``solidAngle(index, observer)`` and ``componentType(index)`` (returning a :class:`~mantid.geometry.ComponentType`) to python. These provide index-based replacements for ``IDetector.solidAngle`` and the ``RectangularDetector``/``GridDetector`` type checks, as part of the migration to the ``ComponentInfo``/``DetectorInfo`` access layers.
- (`#41801 <https://github.com/mantidproject/mantid/pull/41801>`_) :class:`~mantid.api.MatrixWorkspace` now exposes
  the preferred ``Histogram`` data accessors to python:
  ``x()``, ``y()``, ``e()``, ``dx()`` (read-only numpy wrappers) and
  ``mutableX()``, ``mutableY()``, ``mutableE()``, ``mutableDx()``
  (writable numpy wrappers). These replace the legacy ``readX()``/``readY()``/``readE()``/``readDx()``
  and ``dataX()``/``dataY()``/``dataE()``/``dataDx()`` methods,
  which are now deprecated and emit a ``DeprecationWarning`` naming the replacement,
  but continue to work and return the same data.
  See the :ref:`HistogramData` concept page for more information on the ``Histogram`` data model.
- (`#41801 <https://github.com/mantidproject/mantid/pull/41801>`_) ``dx()``/``mutableDx()`` behave the same way ``readDx()``/``dataDx()`` always have:
  if no Dx (x-error) data has been set on the spectrum, it is first initialized to zeros,
  so these methods are always safe to call.
- (`#41801 <https://github.com/mantidproject/mantid/pull/41801>`_) The deprecation warnings above are emitted via
  `PyErr_Warn(PyExc_DeprecationWarning, ...) <https://docs.python.org/3/c-api/exceptions.html#c.PyExc_DeprecationWarning>`__,
  equivalent to ``warnings.warn(message, DeprecationWarning)``,
  and follow Python's standard warnings filtering:
  by default they are printed only for deprecated calls made directly from a user script
  or interactive session (``__main__``), once per call site,
  while calls from inside imported modules are silent.
  Run Python with ``-W default`` (or use ``warnings.simplefilter("default")``) to surface
  every occurrence, or ``error`` in place of ``default`` to raise them as exceptions
  when hunting call sites in tests or CI.
- (`#42181 <https://github.com/mantidproject/mantid/pull/42181>`_) `PanelSurfaceCalculator.getSideBySideViewPos()` will no longer take an instrument parameter.


Dependencies
------------------

New features
############
- (`#41781 <https://github.com/mantidproject/mantid/pull/41781>`_) Removed dependence on the ``quasielasticbayes`` library to facilitate the upgrade to Qt6.
- (`#41974 <https://github.com/mantidproject/mantid/pull/41974>`_) Upgraded python to 3.13. See the changes made to python `here <https://docs.python.org/3/whatsnew/3.13.html>`__.
- (`#42067 <https://github.com/mantidproject/mantid/pull/42067>`_) Automatically set ``MPLCONFIGDIR`` to ``$XDG_CACHE_HOME/matplotlib`` when ``XDG_CACHE_HOME`` is set
- (`#42108 <https://github.com/mantidproject/mantid/pull/42108>`_) Remove ArielToMantidXML tool. This is a tool that was used when transitioning instruments to mantid from other reduction software.

Bugfixes
############


MantidWorkbench
---------------

See :doc:`mantidworkbench`.

:ref:`Release 7.0.0 <v7.0.0>`
