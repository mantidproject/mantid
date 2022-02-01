=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Improvements
------------

- Increased the size of the run(s) column for ease of use.
- ``MagnetismReflectometryReduction`` will raise a value error when none of the input list can be processed.

Bugfixes
--------
- The Add Row button is now disabled in the :ref:`Experiment Settings Tab <refl_exp_instrument_settings>` when runs are being processed.
- Invalid Per-Angle default values that are saved into a batch will now be marked as invalid when loaded back into the GUI.
- Fixed un-selected groups being processed after loading a batch.

:ref:`Release 6.3.0 <v6.3.0>`
