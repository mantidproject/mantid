============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. _6_13_sans:

New Features
------------
- A new :ref:`algm-SavePolarizedNXcanSAS` algorithm has been added to save reduced polarized SANS data in the NXcanSAS
  format.

  - :ref:`algm-SANSSave` now supports the new :ref:`algm-SavePolarizedNXcanSAS` algorithm.
  - :ref:`algm-LoadNXcanSAS` has been modified to load polarized NXcanSAS data saved via
    :ref:`algm-SavePolarizedNXcanSAS`.
  - Polarization options have been added the :ref:`sans_toml_v1-ref` format. The most up-to-date version of SANS TOML,
    which supports these options, is now V2.
  - ISIS SANS Batch Reductions (such as those performed by ``WavRangeReduction`` from the
    :ref:`ISIS Command Interface <ScriptingSANSReductions>`) can now use polarization properties defined in the V2 User
    File to save using :ref:`algm-SavePolarizedNXcanSAS`.

- The :ref:`ISIS Command Interface <ScriptingSANSReductions>` now allows a save algorithm to be passed directly to the
  ``WavRangeReduction`` command.
- The documentation for :ref:`scripting SANS reduction<ScriptingSansReductions>` has been updated with more information
  about using the command line interface.
- The ``SANSadd2`` and ``CreateSANSWavelengthPixelAdjustment`` functions have been modified to enable the summing of LOQ
  event mode data, and allow for the use of the new LOQ IDF.

Bugfixes
--------
- :ref:`ISIS_SANS_Sum_Runs_Tab-ref` in the ISIS SANS interface now maintains the sample properties (shape, thickness,
  height, and width) of the runs in the output file after performing a sum.

:ref:`Release 6.13.0 <v6.13.0>`
