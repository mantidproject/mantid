=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
----------------------------
New
###
- Batch Save/Load: full saving the runs table and all of the related settings for a batch is now possible.

Improved
########

- The polarization correction inputs have been simplified to a single checkbox which when ticked will apply polarization corrections based on properties in the instrument parameters file.

Algorithms
----------

New
###

- :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto-v3>` has been rewritten and updated to version 3. In the new version the polarization correction properties have been removed from the algorithm input and are now taken from the parameter file. A checkbox has been added to indicate whether the corrections should be applied.

Improved
########

- Grouping of groups has been avoided by adding individual workspaces from input group into the TOF group and removing the original group, ignoring any workspaces with x-axes that are not TOF.

Bug fixes
#########

The following bugs have been fixed since the last release:

- The pause button is now disabled upon opening the interface and becomes enabled when a process starts.

:ref:`Release 4.2.0 <v4.2.0>`
