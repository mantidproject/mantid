=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:


New features
------------

- On the ISIS Reflectometry interface, you can now specify default experiment settings on a per-sample basis as well as a per-angle basis. The new ``Title`` field in the lookup table allows you to specify a regular expression. Any runs whose title matches this expression will use those settings by default.
- Groups are now highlighted to show that all subtasks are completed.
- Processing is disabled if there are errors on the Experiment Setting table.
- Removed the automated template generation from the `LRAutoReduction` algorithm, which was never used.
- Removed the so-called "primary fraction correction" from the `LRAutoReduction` algorithm. This correction is no longer in use.
- Renamed the per-angle defaults table to the Settings Lookup table to better represent that lookups can now be done by both angle and by sample title.
- The Runs Tab of the ISIS Reflectometry Interface now contains a Lookup Index column to indicate which of the rows from the Experiment Settings tab was used.
- The ISIS Reflectometry interface now log a warning if starting live data will use potentially unexpected settings
- Updated instrument selector tooltips to clarify that changes to this setting apply across all of mantid.

Bugfixes
--------

- Loading a batch now completely clears the Experiment Settings tab's lookup table before re-populating it.
- Fixed a potential crash running live data reduction when SliceViewer is open on the live data workspace.
- Workbench will no longer crash if ReflectometryBackgroundSubtraction is run with a group workspace when using the Algorithm dialog.
- Fix a problem where the default processing instructions could be incorrect. Sensible default values are now used for the INTER linear detector if not specified by the user. Previously, an incorrect pattern was being specified resulting in confusing errors such as a reduced workspace with multiple histograms or error messages about invalid detector IDs or angle correction not being possible.
- Fix a bug where Mantid could hang when running multiple python algorithms simultaneously, e.g. when running live data and processing a batch at the same time.

:ref:`Release 6.4.0 <v6.4.0>`
