=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------
Corrupted Instrument Definitions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning:: Instruments in Mantid will no longer silently discard detectors defined with duplicate IDs. This has been a long-standing source of hard to find issue in Mantid. We have endeavoured to ensure that all :ref:`Instrument Definition Files <InstrumentDefinitionFile>` shipped with Mantid are now corrected. **If you have local IDFs, please update them to remove any duplicate IDs, or ask the Mantid Team for help**, the warning and error information in Mantid will give details about duplicates in IDFs that cannot be loaded.

    Beware that :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` will now **NOT load the Instrument from historic Processed Nexus files if the embedded IDF is corruped with duplicate detector IDs**. The workspace (data) part will still load, but the workspace will not have any Instrument attached. There are new warnings generated that describe the exact problem and the remedy when this happens. Note that the fix is generally easy. You should be able to run :ref:`LoadInstrument <algm-LoadInstrument>` on the Workspace, pointing it to an updated IDF, which is free of duplicates.

Please contact the Mantid Team if you experience any further problems as a result of these changes.

Algorithms
----------

:ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports workspaces with detector scans and workspaces with single-count point data.
- :ref:`CreateWorkspace <algm-CreateWorkspace>` will no longer create a default (and potentially wrong) mapping from spectra to detectors, unless a parent workspace is given. This change ensures that accidental bad mappings that could lead to corrupted data are not created silently anymore. This change does *not* affect the use of this algorithm if: (1) a parent workspace is given, or (2) no instrument is loaded into to workspace at a later point, or (3) an instrument is loaded at a later point but ``LoadInstrument`` is used with ``RewriteSpectraMapping=True``. See also the algorithm documentation for details.
- :ref:`Fit <algm-Fit>` will now respect excluded ranges when ``CostFunction = 'Unweighted least squares'``.
- :ref:`NormaliseToMonitor <algm-NormaliseToMonitor>` now supports non-constant number of bins.

Core Functionality
------------------

- Fixed an issue where certain isotopes could not be accessed using the `Atom` classes, e.g Si28.

Performance
-----------

- Improved performance for second and consecutive loads of instrument geometry, particularly for instruments with many detector pixels. This affects :ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` and load algorithms that are using it.

Python
------
In `mantid.simpleapi`, a keyword has been implemented for function-like algorithm calls to control the storing on the Analysis Data Service.
`StoreInADS=False` can be passed to function calls to not to store their output on the ADS.

- The ``isDefault`` attribute for workspace properties now works correctly with workspaces not in the ADS.

:ref:`Release 3.12.0 <v3.12.0>`
