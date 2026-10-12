=========================
Indirect Geometry Changes
=========================

New Features
------------
- (`#40747 <https://github.com/mantidproject/mantid/pull/40747>`_) :ref:`ISIS Energy Transfer <ISISEnergyTransfer>` in the :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` interface now supports OSIRIS silicon analyser reductions for the 111 and 333 reflections, with detector-tube and angular grouping options and NXSPE export.
- (`#41919 <https://github.com/mantidproject/mantid/pull/41919>`_) `ISISDiagnostics` tab of the :ref:`Data Reduction <interface-indirect-data-reduction>` interface now has a `Sum Files` checkbox to average a comma separate list of input runs.
- (`#41919 <https://github.com/mantidproject/mantid/pull/41919>`_) `Transmission` tab of the :ref:`Data Reduction <interface-indirect-data-reduction>` interface now has a `Sum Files` checkbox to average a comma separate list of sample or can runs.


Bugfixes
--------
- (`#41941 <https://github.com/mantidproject/mantid/pull/41941>`_) The output workspace name for summed runs in the  ``ISISCalibration`` tab of the :ref:`Indirect Data Reduction<interface-indirect-data-reduction>` is formatted correctly.


Algorithms
----------

New features
############

Bugfixes
############

:ref:`Release 7.0.0 <v7.0.0>`
