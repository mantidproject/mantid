=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- (`#40523 <https://github.com/mantidproject/mantid/pull/40523>`_) The :ref:`interface-isis-refl` will now apply
  angle-title specific settings from the lookup table on :ref:`Experiment Settings <refl_exp_instrument_settings>` tab
  when processing live data. Previously it was necessary to ensure a wildcard row was present in the table that would be
  applied uniformly to all live data.

:ref:`Release 6.15.0 <v6.15.0>`
