=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. figure:: ../../../../images/mantid_workbenchnightly.png
   :class: screenshot
   :width: 200px
   :align: right

Installation
------------

- **The macOS bundle is now suffixed with** ``Nightly`` **if it comes from a nightly development build and so can be installed alongside a full release build.**

Concepts
--------

- Sphinx documentation now uses MathJax to render equations
- Time-dependent values for individual instrument parameters are now allowed
- Added a ``PythonStdOutChannel`` and the ability to change the :ref:`logging channel <Properties File>` without restarting mantid framework
- Added packing fraction to :ref:`Material <Materials>` for separate and effective number density
- Bugfix :ref:`DownloadInstrument <algm-DownloadInstrument>` to try unauthenticated connections if authenticated failes. Also added the ability to allow for setting the github api token in the :ref:`properties file <Properties File>`

Algorithms
----------

- :ref:`SetSample <algm-SetSample>` and :ref:`SetSampleMaterial <algm-SetSampleMaterial>` allow setting the packing fraction and effective number density
- The calculation of distance is now correct for tracks passing through objects that have multiple intercepting surfaces, e.g. hollow cylinder. This affects algorithms such as :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>`.
- :ref:`AlignComponents <algm-AlignComponents>` now minimizes a set of peak-center deviations in d-spacing, instead of the geometrical DIFC parameters.
- :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>` now produces a ragged workspace and can now be used on large data sets.
- :ref:`CreateGroupingWorkspace <algm-CreateGroupingWorkspace>` now has the ability to specify a ``CustomGroupingString``.
- :ref:`LoadLamp <algm-LoadLamp>` is corrected to load sample logs with python3.
- :ref:`CalculateEfficiency <algm-CalculateEfficiency>` has a new property ``MergeGroups`` that merges and averages the input group
  Masked spectra will be filled with data (if available) from other entries of the input group instead of processing them
  entry-by-entry. The algorithm now also overwrites the ``processGroups`` method. These features aide the calculation of sensitivity maps.

Fitting
-------

- Added button to clear all custom setups in ``Setup > Manage Setups`` menu
- Corrected a bug in the calculation of uncertainty bands on the calculated fit curve. This correction showed agreement with the python fitting package ``kmpfit``.

Python
------

- Created a new module :ref:`mantid.utils <mantid.utils>` to allow for code sharing between algorithms.
- :py:meth:`mantid.api.IMDWorkspace.getNumNonIntegratedDims` returns the number of non-integrated dimensions present.

- The reliability of Mantid downloading Instrument data from the GitHub API has been improved. Mantid tries an initial authenticated call and falls back on an anonymous call. The anonymous call wasn't working properly and this has been fixed.
- Give warning when instrument in Facilities.xml has errors

- Error log messages (e.g. ``dx= 0.25 0.251 12``) from an EqualBinChecker are now no longer produced when editing python scripts, if a workspace is present with unequal bin sizes
- Warning log messages from the InstrumentValidator are no longer produced when editing some python scripts.

- Setting the same axis to multiple workspaces clones the axis, avoiding a possible crash.


:ref:`Release 6.0.0 <v6.0.0>`
