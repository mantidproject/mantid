============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- :ref:`algm-SaveNXcanSAS` has been extended to support the saving of multiple file formats. This functionality will be exposed via the ISIS SANS Command Interface in a following release.
- The LARMOR and ZOOM Instrument Definition Files have been updated to include polarizing components.

Bugfixes
--------
- :ref:`algm-PolarizerEfficiency` will no longer crash when passing in a ``SpinStates`` string that contains more spin states than the number of workspaces in the input workspace group.

:ref:`Release 6.12.0 <v6.12.0>`
