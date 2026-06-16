=======================
Direct Geometry Changes
=======================

General
-------

New features
############
- (`#41309 <https://github.com/mantidproject/mantid/pull/41309>`_) In :ref:`DGS Planner <dgsplanner-ref>`, the minimal value for the Incident Energy has been lowered from 1.0 to 0.1 meV.
- (`#41305 <https://github.com/mantidproject/mantid/pull/41305>`_) CNCS has a new parameter file to update the calculation of T0 offset.

MSlice
------

New features
############
- Updated ``matplotlib`` to version 3.10.
- Updated ``python`` to version 3.12.
- Added an error message to prevent export to ``.nxspe`` when workspace limits and the axes limits do not match.

Bugfixes
############
- Improved robustness against missing workspaces after clearing the ADS in Mantid.

DNS_Reduction
-------------

New features
############
- (`#40088 <https://github.com/mantidproject/mantid/pull/40088>`_) The :ref:`DNS_Reduction <dns_reduction-ref>` GUI now includes functionality for data reduction of single crystal elastic data.

:ref:`Release 6.16.0 <v6.16.0>`
