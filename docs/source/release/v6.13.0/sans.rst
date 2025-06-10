============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- :ref:`algm-SANSSave` now supports the new :ref:`algm-SavePolarizedNXcanSAS` algorithm.
- :ref:`algm-LoadNXcanSAS` can now load polarized NXcanSAS data.
- Changes have been made to ``SANSadd2`` and ``CreateSANSWavelengthPixelAdjustment`` to enable the summing of LOQ event mode data, and allow for the use of the new LOQ IDF.
- ISIS SANS Batch Reductions (such as those performed by ``WavRangeReduction`` from the
  :ref:`ISIS Command Interface <ScriptingSANSReductions>`) can now make use of polarization
  properties defined in the V2 User File to save using :ref:`algm-SavePolarizedNXcanSAS`.
- Polarization options have been added the :ref:`sans_toml_v1-ref` format.
  As a result, the most up-to-date version of SANS TOML, which supports these options, has been bumped to 2.
- Update SANS Command Line Interface to pass a save algorithm directly to the ``WavRangeReduction`` command.
- Update documentation for :ref:`scripting SANS reduction<ScriptingSansReductions>` using command line interface.
- New algorithm :ref:`algm-ReflectometryISISCalculatePolEff` added. This algorithm provides a convenient wrapper for the :ref:`algm-ReflectometryISISCreateTransmission`, :ref:`algm-PolarizationEfficienciesWildes` and :ref:`algm-JoinISISPolarizationEfficiencies` algorithms.
- Add new :ref:`algm-SavePolarizedNXcanSAS` algorithm to save reduced polarized SANS data in NXcanSAS format.

Bugfixes
--------
- :ref:`ISIS_SANS_Sum_Runs_Tab-ref` in the ISIS SANS interface now maintains the sample properties
  (shape, thickness, height, and width) of the runs in the added output file.

:ref:`Release 6.13.0 <v6.13.0>`
