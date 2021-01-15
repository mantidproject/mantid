=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------

- The macOS bundle is now suffixed with ``Nightly`` if it comes from a nightly development build and can be installed alongside a full release build.

Concepts
--------

- Change sphinx documentation to use mathjax to render equations
- Added packing fraction to :ref:`Material <Materials>` to separate number density and effective number density.
- Added a feature allowing time-dependent values for individual instrument parameters.
- Added a ``PythonStdOutChannel`` and the ability to change the :ref:`logging channel <Properties File>` without restarting mantid framework

Algorithms
----------

- The calculation of a distance has been updated in Track to correctly calculate the distance for objects that have multiple intercepting surfaces, e.g. hollow cylinder. This affect algorithms such as :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` where you may now get slightly different values.
- Added the ability to specify the packing fraction and effective number density to :ref:`SetSample <algm-SetSample>` and :ref:`SetSampleMaterial <algm-SetSampleMaterial>`.
- :ref:`AlignComponents <algm-AlignComponents>` now minimizes a set of peak-center deviations in d-spacing, instead of the geometrical DIFC parameters.
- :ref:`CropWorkspaceRagged <algm-CropWorkspaceRagged>` now produces ragged workspace and can now be used on large data sets.
- Added the ability to specify a ``CustomGroupingString`` when creating a detector grouping workspace using :ref:`CreateGroupingWorkspace <algm-CreateGroupingWorkspace>`.
- :ref:`LoadLamp <algm-LoadLamp>` is corrected to load sample logs as well under python3.

Fitting
-------

- Corrected a bug in the calculation of uncertainty bands on the calculated fit curve. This correction has been tested against the python fitting package ``kmpfit``, where an agreement between the two was seen.
- Added button to clear all custom setups in Setup > Manage Setups menu

Data Objects
------------

Python
------

- Created a new module :ref:`mantid.utils <mantid.utils>` to allow for code sharing between algorithms.

Improvements
############
- Member function: MDGeometry::getNumNonIntegratedDims() returns the number of non-integrated dimensions present.
- When Mantid interacts with the GitHub API it tries an initial authenticated call and if that fails for any reason a fallback anonymous call is made. The anonymous call wasn't working properly and this has been fixed. This provides some extra reliability for processes such as the Instrument data download that is performed during startup of Workbench

Bugfixes
########
- Error log messages from an EqualBinChecker are now no longer produced when editing python scripts if a workspace is present with unequal bin sizes
- Warning log messages from the InstrumentValidator are no longer produced when editing some python scripts.
- A bug has been fixed when plotting bin plots on a workspace with numerical axis.
- A bug is fixed when setting the same axis to multiple workspaces, which would cause a crash when deleting the workspaces.
- Give warning when instrument in Facilities.xml has errors

:ref:`Release 6.0.0 <v6.0.0>`
