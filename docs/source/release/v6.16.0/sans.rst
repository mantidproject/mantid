============
SANS Changes
============

New Features
------------
- (`#41273 <https://github.com/mantidproject/mantid/pull/41273>`_) Remove the 'plot results' check box from the ISIS SANS interface which has been hidden since August 2024.
- (`#41168 <https://github.com/mantidproject/mantid/pull/41168>`_) Restrict import of deprecated ISISCommandInterface unless a feature flag is set on the user Mantid Properties file.

Bugfixes
--------
- (`#41372 <https://github.com/mantidproject/mantid/pull/41372>`_) Time slices that fall entirely within a beam-off window (detected via the ``dae_beam_current`` sample log) are now skipped before the merged front+rear reduction, with a clear log message identifying the slice times and run number. Previously such a slice failed deep inside :ref:`SANSFitShiftScale <algm-SANSFitShiftScale>` with a cryptic ``XMax must be greater than XMin`` error and aborted the rest of the batch.
- (`#40881 <https://github.com/mantidproject/mantid/pull/40881>`_) Added more unit tests for the ``SANS`` scripts.

:ref:`Release 6.16.0 <v6.16.0>`
