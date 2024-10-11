=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- New algorithm :ref:`algm-PolarizationEfficienciesWildes` has been added for calculating the efficiencies of the polarizing components of an instrument with two flippers.
  This algorithm implements the approach from the A. R. Wildes 2006 paper.

  .. figure::  ../../images/6_11_release/PolarizationEfficienciesWildes.png
     :width: 500px


Bugfixes
--------
- :ref:`Stitch1DMany <algm-Stitch1DMany-v1>` algorithm now runs successfully via the GUI.
- The orsopy library has been updated to version 1.2.1.
  This pulls in a bug fix where multi-dataset reduced reflectivity ``.ort`` files with different column headers for different datasets would use only the headers for the first dataset throughout the file.
- Algorithm :ref:`algm-ReflectometryReductionOneLiveData` now applies polarization corrections correctly on ``WorkspaceGroup`` inputs.
- When on IDAaaS, only runs that are available in the ISIS Instrument Data Cache will now appear in the ``Search Runs``
  section of the :ref:`Runs tab <refl_runs>` when the ``Search Data Archive`` setting is set to ``Off``.


:ref:`Release 6.11.0 <v6.11.0>`
