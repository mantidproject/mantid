============
Muon Changes
============

.. contents:: Table of Contents
   :local:

ALC
---

Bugfixes
############
- :ref:`ALC interface <MuonALC-ref>` no longer crashes when non numeric characters are input in Backward and Forward Grouping line edits.

Muon Analysis
-------------

Bugfixes
############
- Fix a crash occurring in the :ref:`Results Tab <muon_results_tab-ref>` of the :ref:`Muon Analysis <Muon_Analysis-ref>`
  interface when an integer time series was added as a log in the results table.
- Allow the ``current_period`` sample log from the selected fitted workspace to be added to the
  :ref:`results table <muon_results_tab-ref>`.

Algorithms
----------

Bugfixes
############
- Improved the numerical stability of :ref:`PhaseQuad <algm-PhaseQuad-v1>`. This algorithm was inverting a 2x2 matrix manually, which resulted in numerically unstable results. Performance should also be improved.

:ref:`Release 6.13.0 <v6.13.0>`
