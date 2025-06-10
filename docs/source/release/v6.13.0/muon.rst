============
Muon Changes
============

.. contents:: Table of Contents
   :local:


Frequency Domain Analysis
-------------------------

Bugfixes
############



Muon Analysis
-------------

Bugfixes
############
- Fix a crash ocurring in the :ref:`Results Tab <muon_results_tab-ref>` of the :ref:`Muon Analysis <Muon_Analysis-ref>` interface when an integer time series was added as a log in the results table.
- Allow the `current_period` sample log from the selected fitted workspace to be added to the :ref:`results table <muon_results_tab-ref>`.


Muon Analysis and Frequency Domain Analysis
-------------------------------------------

Bugfixes
############



ALC
---

Bugfixes
############



Elemental Analysis
------------------

Bugfixes
############



Algorithms
----------

Bugfixes
############
- Improved the numerical stability of :ref:`PhaseQuad <algm-PhaseQuad-v1>`. This algorithm was inverting a 2x2 matrix manually, which resulted in numerically unstable results. Performance should also be improved.

:ref:`Release 6.13.0 <v6.13.0>`
