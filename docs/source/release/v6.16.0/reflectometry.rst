=====================
Reflectometry Changes
=====================

New Features
------------
- (`#41325 <https://github.com/mantidproject/mantid/pull/41325>`_) A new version of :ref:`algm-ReflectometryReductionOne-v3` has been introduced. This version of the algorithm is tasked based, and therefore a new property ``TaskExecutionOrder`` can be specified, enabling advanced users to customise the algorithm workflows.
- (`#41335 <https://github.com/mantidproject/mantid/pull/41335>`_) :ref:`algm-ReflectometryReductionOneAuto` has been refactored to make use of the new ``TaskExecutionOrder`` property in :ref:`algm-ReflectometryReductionOne` to control the flow of the algorithm to avoid task repetition in the Polarization Correction workflow.
- (`#41345 <https://github.com/mantidproject/mantid/pull/41345>`_) :ref:`algm-ReflectometryReductionOneAuto` now records its algorithm history when reducing ``WorkspaceGroup`` inputs using polarization analysis. The resulting workspace history records the calling parent algorithm, with ``ReflectometryReductionOneAuto`` and the underlying ``ReflectometryReductionOne`` reductions recorded as child histories. :ref:`algm-ReflectometryISISLoadAndProcess` now uses this path when it reduces group inputs.
- (`#41415 <https://github.com/mantidproject/mantid/pull/41415>`_) The :ref:`Reduction Preview <refl_preview>` tab in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now supports creating mask and inverted mask tables using rectangular, elliptical and polygonal selectors.
- (`#41074 <https://github.com/mantidproject/mantid/pull/41074>`_) The :ref:`Reduction Preview <refl_preview>` tab in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now has a checkbox to toggle the Y-axis scale between log and symlog. An input field has also been added to set the linthresh value.
- (`#41425 <https://github.com/mantidproject/mantid/pull/41025>`_) The :ref:`Save <refl_save>` tab in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>` now has an additional text box to save the model description metadata into ORSO file formats. The model description can also be optionally validated.

Bugfixes
--------
- (`#41360 <https://github.com/mantidproject/mantid/pull/41360>`_) The :ref:`algm-PolarizationEfficienciesWildes` algorithm now accounts for covariance terms when calculating a
  polarizer or analyser efficiency from the other supplied efficiency workspace, avoiding overestimated propagated
  errors when the supplied efficiency is a derived quantity.

:ref:`Release 6.16.0 <v6.16.0>`
