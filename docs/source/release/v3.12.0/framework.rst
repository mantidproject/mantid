=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Instrument Definition Updates
-----------------------------

- The MAPS IDF has been updated following its upgrade.

Instrument Definitions
######################
.. warning:: **Users:** Instruments in Mantid will no longer silently discard detectors defined with duplicate IDs. This has been a long-standing source of hard to find issue in Mantid. We have endeavoured to ensure that all :ref:`Instrument Definition Files <InstrumentDefinitionFile>` shipped with Mantid are now corrected. **If you have local IDFs, please update them to remove any duplicate IDs, or ask the Mantid Team for help**, the warning and error information in Mantid will give details about duplicates in IDFs that cannot be loaded.

    Beware that :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` will now **NOT load the Instrument from historic Processed Nexus files if the embedded IDF is corruped with duplicate detector IDs**. The workspace (data) part will still load, but the workspace will not have any Instrument attached. There are new warnings generated that describe the exact problem and the remedy when this happens. Note that the fix is generally easy. You should be able to run :ref:`LoadInstrument <algm-LoadInstrument>` on the Workspace, pointing it to an updated IDF, which is free of duplicates.

Please contact the Mantid Team if you experience any further problems as a result of these changes.


Algorithms
----------

New
###
- :ref:`MostLikelyMean <algm-MostLikelyMean>` is a new algorithm that computes the mean of the given array, that has the least distance from the rest of the elements.
- :ref:`LoadAndMerge <algm-LoadAndMerge>` is a new algorithm that can load and merge multiple runs.
- :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>`: New algorithm that will crop each spectrum with a different x-range.
- :ref:`LoadLamp <algm-LoadLamp>`: New algorithm to load processed HDF5 files produced by LAMP program at ILL.
- :ref:`SaveReflections <algm-SaveReflections>` is a new algorithm to save PeaksWorkspaces to Fullprof, Jana, GSAS, and SHELX text formats.

Improved
########
- :ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports workspaces with detector scans and workspaces with single-count point data.
- :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`: It is now possible to choose between weighted and unweighted fitting.
- :ref:`CreateWorkspace <algm-CreateWorkspace>` will no longer create a default (and potentially wrong) mapping from spectra to detectors, unless a parent workspace is given. This change ensures that accidental bad mappings that could lead to corrupted data are not created silently anymore. This change does *not* affect the use of this algorithm if: (1) a parent workspace is given, or (2) no instrument is loaded into to workspace at a later point, or (3) an instrument is loaded at a later point but ``LoadInstrument`` is used with ``RewriteSpectraMapping=True``. See also the algorithm documentation for details.
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` now supports non-constant bins.
- :ref:`Fit <algm-Fit>` will now respect excluded ranges when ``CostFunction = 'Unweighted least squares'``.
- :ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports a non-constant number of bins.
- :ref:`CompressEvents <algm-CompressEvents>` now supports compressing events with pulse time.
- :ref:`MaskBins <algm-MaskBins>` now uses a modernized and standardized way for providing a list of workspace indices. For compatibility reasons the previous ``SpectraList`` property is still supported.
- :ref:`LoadMcStas <algm-LoadMcStas>` now loads event data in separate workspaces (single scattering, multiple scattering) as well as all scattering.
- :ref:`MaxEnt <algm-MaxEnt>`: The ``EvolChi`` and  ``EvolAngle`` workspaces now only contain data up until the result has converged.
- :ref:`SaveNexus <algm-SaveNexus>` will no longer crash when passed a ``PeaksWorkspace`` with integrated peaks that have missing radius information.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` will now accept workspaces with varying x-axes per spectrum.
- :ref:`LoadEXED <algm-LoadEXED>` has better handling of monitor workspace and sample logs.

Bugfixes
########
- :ref:`Fit <algm-Fit>` has had a bug fixed that prevented a fix from being removed.
- :ref:`LoadMask <algm-LoadMask>` has had a bug fixed that could, under certain conditions, cause detectors from previously loaded masking to be added to the currently loaded masking.

Known Issues
############
- :ref:`LoadEventNexus <algm-LoadEventNexus>` is incorrectly ignoring the `FilterMonBy*` properties. When loading monitors as events the output
  `*_monitors` workspace then contains all recorded events rather than those accepted by the filters. To work around this issue run the
  :ref:`FilterByTime <algm-FilterByTime>` algorithm on the output `*_monitors` workspace with the same values as passed to the `FilterMonBy*`
  properties.

Fitting
-------
Improved
########
- :ref:`EISFDiffSphere <func-EISFDiffSphere>` fits the Q-dependence on the EISF of a particle undergoing continuous diffusion but confined to a spherical volume.
- :ref:`EISFDiffSphereAlkyl <func-EISFDiffSphereAlkyl>` fits the Q-dependence on the EISF of an alkyl molecule, like a membrane lipd.
- :ref:`EISFDiffCylinder <func-EISFDiffCylinder>` models the elastic incoherent scattering intensity of a particle diffusing within a cylinder.

Bugfixes
########
- Fix for a bug in calculating numerical derivatives by applying ties correctly.

Core Functionality
------------------

New
###
- Added new classes ``ConfigObserver`` for listening for changes to any configuration property and ``ConfigPropertyObserver`` for listening to changes to an individual config property of interest.

Improved
########
- :class:`mantid.kernel.FloatTimeSeriesProperty` now returns :class:`numpy.datetime64` for the log times.
- The duration reported by a running algorithm now includes time spent for validation of properties and inputs. This fixes a discrepancy between observed and reported timings if validation is expensive, e.g., when checking if a file exists. More detailed timing information is now available when setting the log level to ``debug``.
- The status of a fit in the fit window is now at the top of the of the dialog instead of the bottom.
- Condition to check if a property is enabled when serializing.
- Workspace locking no longer prevents simple read operations required to display the workspace conext menu in Mantidplot.
- TableWorkspaces can now be converted to a Python dictionary by calling the ``table.toDict()`` function.
- ``MultiFileProperty`` now accepts complex summation ranges for run numbers, such as ``111-113+115`` and ``111-115+123-132``.

Bugfixes
########
- Fixed an issue where certain isotopes could not be accessed using the `Atom` classes, e.g Si28.
- ``datasearch.searcharchive`` :ref:`property <Properties File>` has new functionality to only search the default facility.
- Fixed the calculation of scattering length and scattering length squared for :py:obj:`Material <mantid.kernel.Material>`.
- Fixed the behaviour of ``UpdateInstrumentDefinitions.OnStartup`` in the :ref:`properties file <Properties File>`. It was not being used correctly for using the updated ``Facilities.xml`` file.


Live Data
---------

New
###
- ``KafkaEventListener`` is a new live listener for neutron event and sample environment data which is in development for the ESS and ISIS.

Performance
-----------

Improved
########
- :ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` and load algorithms that are using it. Improved performance for second and consecutive loads of instrument geometry, particularly for instruments with many detector pixels. 
- :ref:`CropToComponent <algm-CropToComponent>`: Up to 30% performance improvement, based on ongoing work on Instrument-2.0.
- :ref:`MaxEnt <algm-MaxEnt>`: Improved rate of convergence. The  ``ChiTarget`` property has been replaced by  ``ChiTargetOverN``.

Bugfixes
########
- A `bug <https://github.com/mantidproject/mantid/pull/20953>`_ in the handling of fractional bin weights in a specialised form (`RebinnedOutput <http://doxygen.mantidproject.org/nightly/d4/d31/classMantid_1_1DataObjects_1_1RebinnedOutput.html>`_) of :ref:`Workspace2D <Workspace2D>` has been fixed. This mainly affects the algorithms :ref:`algm-SofQWNormalisedPolygon` and :ref:`algm-Rebin2D`, which underlies the `SliceViewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`_.

Python
------

New
###
- ``mantid.api.IPeak`` has three new functions:
    - ``getEnergyTransfer`` which returns the difference between the initial and final energy.
    - ``getIntensityOverSigma`` which returns the peak intensity divided by the error in intensity.
    - ``getGoniometerMatrix`` which returns the goniometer rotation matrix associated with the peak.


Improved
########


- In ``mantid.simpleapi``, a keyword has been implemented for function-like algorithm calls to control the storing on the Analysis Data Service.
- ``StoreInADS=False`` can be passed to function calls to not to store their output on the ADS.
- The standard Python operators, e.g. ``+``, ``+=``, etc., now work also with workspaces not in the ADS.
- The ``isDefault`` attribute for workspace properties now works correctly with workspaces not in the ADS.
- The previously mentioned ``ConfigObserver`` and ``ConfigPropertyObserver`` classes are also exposed to Python.
- ``mantid.kernel.V3D`` vectors now support negation through the usual ``-`` operator.
- It is now possible to `pickle <https://docs.python.org/2/library/pickle.html>`_ and de-pickle :ref:`Workspace2D <Workspace2D>` and :ref:`TableWorkspace <Table Workspaces>` in Python. This has been added to make it easier to transfer your workspaces over a network. Only these two workspace types currently supports the pickling process, and there are limitations to be aware of described :ref:`here <Workspace2D>`.
- Support for unicode property names has been added to Python. This means that one can run the following in Python2 or Python3.

.. code-block:: python

   from mantid.simpleapi import *
   import json
   source = json.loads('{"Filename":"CNCS_7860_event.nxs"}')
   props = json.loads('{"InputWorkspace":"eventWS", "Params":"1000"}')
   eventWS = Load(**source)
   rebinned = Rebin(**props)

Deprecated
##########

- `MantidPlot.pyplot <http://docs.mantidproject.org/v3.11.0/api/python/mantidplot/pyplot/index.html>`_ was an early attempt to provide Matplotlib style syntax over Mantidplot plotting.  This will be replaced in Mantid 4.0 with MatPlotlib itself, and this packages would cause namespace clashes and confusion.  This package is now deprecated, and will not be included in future releases of Mantid.  To the best of our knowledge the impact of this should be minimal as it is at best only rarely used.


Bugfixes
########
- Fixed an issue with coercing data from python lists or numpy arrays where the datatype!=float64 into a workspace

:ref:`Release 3.12.0 <v3.12.0>`
