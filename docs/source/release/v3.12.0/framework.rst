=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Instrument Definition Updates
-----------------------------

- The MAPS IDF has been updated following its upgrade.

Concepts
--------
Corrupted Instrument Definitions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning:: Instruments in Mantid will no longer silently discard detectors defined with duplicate IDs. This has been a long-standing source of hard to find issue in Mantid. We have endeavoured to ensure that all :ref:`Instrument Definition Files <InstrumentDefinitionFile>` shipped with Mantid are now corrected. **If you have local IDFs, please update them to remove any duplicate IDs, or ask the Mantid Team for help**, the warning and error information in Mantid will give details about duplicates in IDFs that cannot be loaded.

    Beware that :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` will now **NOT load the Instrument from historic Processed Nexus files if the embedded IDF is corruped with duplicate detector IDs**. The workspace (data) part will still load, but the workspace will not have any Instrument attached. There are new warnings generated that describe the exact problem and the remedy when this happens. Note that the fix is generally easy. You should be able to run :ref:`LoadInstrument <algm-LoadInstrument>` on the Workspace, pointing it to an updated IDF, which is free of duplicates.

Please contact the Mantid Team if you experience any further problems as a result of these changes.

Algorithms
----------

- :ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports workspaces with detector scans and workspaces with single-count point data.
- It is now possible to choose between weighted and unweighted fitting in :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`.
- :ref:`CreateWorkspace <algm-CreateWorkspace>` will no longer create a default (and potentially wrong) mapping from spectra to detectors, unless a parent workspace is given. This change ensures that accidental bad mappings that could lead to corrupted data are not created silently anymore. This change does *not* affect the use of this algorithm if: (1) a parent workspace is given, or (2) no instrument is loaded into to workspace at a later point, or (3) an instrument is loaded at a later point but ``LoadInstrument`` is used with ``RewriteSpectraMapping=True``. See also the algorithm documentation for details.
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` now supports non-constant bins.
- :ref:`Fit <algm-Fit>` will now respect excluded ranges when ``CostFunction = 'Unweighted least squares'``.
- :ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports non-constant number of bins.
- :ref:`MostLikelyMean <algm-MostLikelyMean>` is a new algorithm that computes the mean of the given array, that has the least distance from the rest of the elements.
- :ref:`LoadAndMerge <algm-LoadAndMerge>` is a new algorithm that can load and merge multiple runs.
- :ref:`CompressEvents <algm-CompressEvents>` now supports compressing events with pulse time.
- :ref:`MaskBins <algm-MaskBins>` now uses a modernized and standardized way for providing a list of workspace indices. For compatibility reasons the previous ``SpectraList`` property is still supported.
- :ref:`Fit <algm-Fit>` has had a bug fixed that prevented a fix from being removed.
- :ref:`LoadMcStas <algm-LoadMcStas>` now loads event data in separate workspaces (single scattering, multiple scattering) as well as all scattering.
- :ref:`LoadMask <algm-LoadMask>` has had a bug fixed that could, under certain conditions, cause detectors from previously loaded masking to be added to the currently loaded masking.
- In :ref:`MaxEnt <algm-MaxEnt>` the ``EvolChi`` and  ``EvolAngle`` workspaces only contain data up until the result has converged.
- New algorithm :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>` will crop each spectrum with a different x-range
- :ref:`LoadLamp <algm-LoadLamp>` is a new algorithm to load processed HDF5 files produced by LAMP program at ILL.

Fitting
-------
- :ref:`EISFDiffSphere <func-EISFDiffSphere>` fits the Q-dependence on the EISF of a particle undergoing continuous diffusion but confined to a spherical volume.
- :ref:`EISFDiffSphereAlkyl <func-EISFDiffSphereAlkyl>` fits the Q-dependence on the EISF of an alkyl molecule, like a membrane lipd.

Core Functionality
------------------

- Fixed an issue where certain isotopes could not be accessed using the `Atom` classes, e.g Si28.
- Added new functionality to ``datasearch.searcharchive`` :ref:`property <Properties File>` to only search the default facility
- The status of a fit in the fit window is now at the top of the of the dialog instead of the bottom.
- Condition to check if a property is enabled when serializing.
- Workspace locking no longer prevents simple read operations required to display the workspace context menu in MantidPlot.
- Added new classes ``ConfigObserver`` for listening for changes to any configuration property and ``ConfigPropertyObserver`` for listening to changes to an individual config property of interest.

Performance
-----------

- Improved performance for second and consecutive loads of instrument geometry, particularly for instruments with many detector pixels. This affects :ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` and load algorithms that are using it.
- Up to 30% performance improvement for :ref:`CropToComponent <algm-CropToComponent>` based on ongoing work on Instrument-2.0.
- Improved rate of convergence for :ref:`MaxEnt <algm-MaxEnt>`. The  ``ChiTarget`` property has been replaced by  ``ChiTargetOverN``.

Python
------
In `mantid.simpleapi`, a keyword has been implemented for function-like algorithm calls to control the storing on the Analysis Data Service.
`StoreInADS=False` can be passed to function calls to not to store their output on the ADS.

- The standard Python operators, e.g. ``+``, ``+=``, etc., now work also with workspaces not in the ADS.
- The ``isDefault`` attribute for workspace properties now works correctly with workspaces not in the ADS.
- The previously mentioned ``ConfigObserver`` and ``ConfigPropertyObserver`` classes are also exposed to python.
- ``mantid.kernel.V3D`` vectors now support negation through the usual ``-`` operator.
- ``mantid.api.IPeak`` has two new functions ``getEnergy`` which returns the difference between the initial and final energy and ``getIntensityOverSigma`` which gets the peak intensity divided by the error in intensity.

Support for unicode property names has been added to python. This means that one can run the following in python2 or python3.

.. code-block:: python

   from mantid.simpleapi import Segfault
   import json
   props = json.loads('{"DryRun":true}')
   Segfault(**props)

- Fixed an issue with coercing data from python lists or numpy arrays where the datatype!=float64 into a workspace

:ref:`Release 3.12.0 <v3.12.0>`
