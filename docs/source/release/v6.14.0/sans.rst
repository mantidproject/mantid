============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- Added new ANSTO Bilby loader :ref:`LoadBBY2 <algm-LoadBBY2>` for the new Bilby file format that supports time stamped events.
- Optimized :ref:`algm-PolarizationCorrectionWildes` to reduce computation time by a factor of 4.
- The option to specify a separate scale factor for the High Angle Bank on compatible instruments (such as LOQ and SANS2D) has been added to the :ref:`sans_toml_v1-ref`.
- New algorithm :ref:`algm-DetermineSpinStateOrder` which takes a Polarised SANS transmission run and attempts to determine the spin state for each run period.
- New algorithm :ref:`algm-SANSISISPolarizationCorrections` to calibrate and correct spin leakage in polarized runs for ISIS instruments ZOOM and LARMOR.
- New algorithm :ref:`algm-AssertSpinStateOrder` which uses :ref:`algm-DetermineSpinStateOrder` to assert the spin state order of a Polarised SANS transmission run. The algorithm can reorder the workspace if the spin state order does not match the expected order.
- Algorithm :ref:`algm-HeliumAnalyserEfficiency` now accepts multiple transmission runs. It calculates for each one the analyser efficiency and fits the helium polarization. It fits all runs to a decay curve to obtain the lifetime of the analyzer.
- Added ``EmptyCellFilename`` as an optional property of :ref:`algm-DepolarizedAnalyserTransmission` to be used instead of ``EmptyCellWorkspace``. :ref:`algm-DepolarizedAnalyserTransmission` will load the provided file with :ref:`algm-LoadNexus` and then use it the same as ``EmptyCellWorkspace``.
- New Algorithm :ref:`algm-HeliumAnalyserEfficiencyTime` calculates the efficiency of a helium analyzer at an arbitrary time when given polarization decay parameters: initial polarization, lifetime, and initial or reference time.

Bugfixes
--------
- Algorithm :ref:`algm-PolarizationEfficiencyCor` no longer crashes Mantid when using workspace groups in the ``InputWorkspaces`` property list.

:ref:`Release 6.14.0 <v6.14.0>`
