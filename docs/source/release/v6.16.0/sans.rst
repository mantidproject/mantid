============
SANS Changes
============

New Features
------------
- (`#41273 <https://github.com/mantidproject/mantid/pull/41273>`_) The :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` has been updated to remove the ``plot results`` check box which has been hidden from users since August 2024.
- (`#41168 <https://github.com/mantidproject/mantid/pull/41168>`_) The import of the depreciated :ref:`ISIS Command Interface <ScriptingSANSReductions>` is restricted unless a feature flag is set on the user Mantid Properties file.

Bugfixes
--------
- (`#41372 <https://github.com/mantidproject/mantid/pull/41372>`_) In the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>`, Time slices that fall entirely within a beam-off window (detected via the ``dae_beam_current`` sample log) are now skipped before the merged front+rear reduction, with a clear log message identifying the slice times and run number. Previously such a slice failed deep inside :ref:`SANSFitShiftScale <algm-SANSFitShiftScale>` with a cryptic ``XMax must be greater than XMin`` error and aborted the rest of the batch.
- (`#40881 <https://github.com/mantidproject/mantid/pull/40881>`_) Increased the test coverage of the ``SANS`` scripts.
- (`#40975 <https://github.com/mantidproject/mantid/pull/40975>`_) :ref:`SaveSESANS <algm-SaveSESANS>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` have been updated to include the decimal point (i.e ``1.0``) when outputting data, even if the data is an integer.
- (`#40856 <https://github.com/mantidproject/mantid/pull/40856>`_) :ref:`GenerateFlatCellWorkspaceLOQ <algm-GenerateFlatCellWorkspaceLOQ>` has been updated so that the output is prepended with the monitor spectra. An option to save the output using :ref:`SaveRKH <algm-SaveRKH>` has also been added (``OutputFlatCellFilePath``).

:ref:`Release 6.16.0 <v6.16.0>`
