=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- New algorithm :ref:`algm-PolarizationEfficienciesWildes` has been added for calculating the efficiencies of the polarizing components of an instrument with two flippers. This algorithm implements the approach from the A. R. Wildes 2006 paper.

Bugfixes
--------
- Fix an issue that prevented the StitchIDMany algorithm from running successfully via the GUI.
- The orsopy library has been updated to version 1.2.1. This pulls in a bug fix where multi-dataset reduced reflectivity .ort files with different column headers for different datasets would use only the headers for the first dataset throughout the file.
- Fix a bug where :ref:`algm-ReflectometryReductionOneLiveData` was not applying polarization corrections because it did not handle WorkspaceGroup inputs correctly.
- When on IDAaaS, only runs that are available in the ISIS Instrument Data Cache will appear in the ``Search Runs``
  section of the :ref:`Runs tab <refl_runs>` when the ``Search Data Archive`` setting is set to ``Off``.

:ref:`Release 6.11.0 <v6.11.0>`