=========================
Indirect Geometry Changes
=========================

New Features
------------
- (`#40747 <https://github.com/mantidproject/mantid/pull/40747>`_) :ref:`ISIS Energy Transfer <ISISEnergyTransfer>` in the :ref:`Indirect Data Reduction <interface-indirect-data-reduction>` interface now supports OSIRIS silicon analyser reductions for the 111 and 333 reflections, with options for detector-tube and angular grouping, and NXSPE export.
- (`#41919 <https://github.com/mantidproject/mantid/pull/41919>`_) The :ref:`ISIS Diagnostics <isis-diagnostics>` tab of the :ref:`Data Reduction <interface-indirect-data-reduction>` interface now has a ``Sum Files`` checkbox to average a comma separate list of input runs.
- (`#41919 <https://github.com/mantidproject/mantid/pull/41919>`_) The :ref:`Transmission <indirect-data-reduction-transmission>` tab of the :ref:`Data Reduction <interface-indirect-data-reduction>` interface now has a ``Sum Files`` checkbox to average a comma separate list of sample or can runs.

Bugfixes
--------
- (`#41941 <https://github.com/mantidproject/mantid/pull/41941>`_) The output workspace name for summed runs in the  ISIS Calibration tab of the :ref:`Indirect Data Reduction<interface-indirect-data-reduction>` is now formatted correctly.


:ref:`Release 7.0.0 <v7.0.0>`
