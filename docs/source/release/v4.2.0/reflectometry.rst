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
- Project Save and Load: full saving of runs table including styling and related settings on a batch by batch level. All batches are saved.
- Integration with Project Recovery: GUI should now recover should recovery successfully recreate all workspaces
- The Slit Calculator dialog can now be accessed from the Tools menu.

Improved
########

- Rows and Groups inside of the Runs table will now have their state reset when the underlying reduced workspace is replaced or renamed
- The polarization correction inputs have been simplified to a single checkbox which when ticked will apply polarization corrections based on properties in the instrument parameters file.
- Batch names will now have a unique number assigned to it, and there will no longer be multiple batches of the same name.
- The Instrument is now synchronised across all Batch tabs.
- Live data monitoring update intervals can now be user-specified.

Bug fixes
#########

- A bug has been fixed where the interface could sometimes not be closed after a failed attempt at starting Autoprocessing.
  
Algorithms
----------

New
###

- :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto-v3>` has been rewritten and updated to version 3. In the new version the polarization correction properties have been removed from the algorithm input and are now taken from the parameter file. A checkbox has been added to indicate whether the corrections should be applied.

Improved
########

Bug fixes
#########

The following bugs have been fixed since the last release:

- The pause button is now disabled upon opening the interface and becomes enabled when a process starts.
- Ensure that the TOF group cannot contain non-TOF workspaces or nested groups (nested groups are not supported so are now flattened into a single group instead).
- An issue has been fixed where the incorrect transmission workspaces were being output when debug is on/off.

:ref:`Release 4.2.0 <v4.2.0>`
