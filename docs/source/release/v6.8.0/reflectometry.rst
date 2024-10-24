=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- The :ref:`algm-MagnetismReflectometryReduction` algorithm now has an option allowing the return of empty reflectivity curves.

Bugfixes
--------
- Mantid no longer crashes when reducing a run on the :ref:`Preview tab <refl_preview>` that matches multiple rows in the :ref:`Experiment Settings tab<refl_exp_instrument_settings>` lookup table. An error message is displayed instead.
- The :ref:`Preview tab <refl_preview>` now matches rows in the :ref:`Experiment tab<refl_exp_instrument_settings>` lookup table by checking both run angle and title. Mantid will also no longer crash if a reduction is triggered on the :ref:`Preview tab <refl_preview>` when no run has been loaded.
- ROI Detector IDs that have been entered into the :ref:`Experiment Settings tab<refl_exp_instrument_settings>` lookup table should now be picked up when reducing a run on the :ref:`Preview tab <refl_preview>` with no region selected from the instrument view plot.
- The row state on the :ref:`Runs tab <refl_runs>` is now reset when settings are updated in the :ref:`Experiment Settings tab<refl_exp_instrument_settings>` lookup table by using the Apply button on the :ref:`Preview tab <refl_preview>`.
- :ref:`algm-ReflectometryReductionOneAuto` no longer throws an error when attempting to run a reduction that applies both a background subtraction and a polarization correction.
- The Q resolution calculation for the custom and TXT file formats output by :ref:`algm-SaveReflectometryAscii` has been corrected so that it matches the calculation described in the documentation.

:ref:`Release 6.8.0 <v6.8.0>`
