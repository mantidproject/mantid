=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- When running the live data monitor from the ISIS Reflectometry interface, it will now be automatically restarted if it stops unexpectedly.
- The Save ASCII tab now has a checkbox to allow individual row outputs to be included as well as group outputs when autosave is selected.
- If both transmission runs are set on the Experiment Settings tab, there was previously no way to override this to only use a single transmission run. This has been changed so that if you specify only the first transmission run on the Runs table, and leave the second blank, then only the first will be used.

Bugfixes
--------
- The lookup index is now set correctly on the ``Runs`` table when transferring from the search results.
- The settings from the ``Event Handling`` tab are now correctly applied after loading a batch.
- Fix a bug with starting the live data monitor from the ISIS Reflectometry interface where there was a delay in updating the processed (IvsQ) workspace.
- The reduction type from the Experiment Settings tab is now ignored when SumInLambda is selected
- Fix a potential crash when performing time slicing if the workspace contains zero counts; you should now get a sensible error message instead.

:ref:`Release 6.5.0 <v6.5.0>`