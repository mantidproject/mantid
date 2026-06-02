=====================
Reflectometry Changes
=====================

New Features
------------
- (`#41335 <https://github.com/mantidproject/mantid/pull/41335>`_) :ref:`algm-ReflectometryReductionOneAuto` has been refactored to make use of the new version of :ref:`algm-ReflectometryReductionOne`. In particular, the ``TaskExecutionOrder`` property is used to control the flow of the algorithm so that tasks are not repeated as part of the Polarization Correction workflow.
- (`#41415 <https://github.com/mantidproject/mantid/pull/41415>`_) The :ref:`Reduction Preview <refl_preview>` tab in the ``ISIS Reflectometry`` interface now supports creating mask and inverted mask tables using rectangular, elliptical and polygonal selectors.
-  The save tab in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now has an additional text box to save the model description metadata into ORSO file formats.
- (`#41074 <https://github.com/mantidproject/mantid/pull/41074>`_) The :ref:`Reduction Preview <refl_preview>` tab in the ``ISIS Reflectometry`` interface now has a checkbox to toggle the Y-axis scale between log and symlog. A line edit has also been added to set the linthresh value.
- (`#41325 <https://github.com/mantidproject/mantid/pull/41325>`_) A new version of :ref:`algm-ReflectometryReductionOne-v3` has been introduced. This version of the algorithm is tasked based, and therefore a new property ``TaskExecutionOrder`` can be specified, enabling advanced users to customise the algorithm workflow.
- (`#41345 <https://github.com/mantidproject/mantid/pull/41345>`_) :ref:`algm-ReflectometryReductionOneAuto` now records its algorithm history when reducing ``WorkspaceGroup`` inputs using polarization analysis. The resulting workspace history records the calling parent algorithm, with ``ReflectometryReductionOneAuto`` and the underlying ``ReflectometryReductionOne`` reductions recorded as child histories. :ref:`algm-ReflectometryISISLoadAndProcess` now uses this path when it reduces group inputs.

Bugfixes
--------
- (`#41360 <https://github.com/mantidproject/mantid/pull/41360>`_) The :ref:`algm-PolarizationEfficienciesWildes` algorithm now accounts for covariance terms when calculating a
  polarizer or analyser efficiency from the other supplied efficiency workspace, avoiding overestimated propagated
  errors when the supplied efficiency is a derived quantity.

:ref:`Release 6.16.0 <v6.16.0>`
