========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Interfaces
----------

New
###

- Added the ability to manually specify a temperature for a set of runs in the TOFTOF reduction dialog.

Improvements
############

- :ref:`PyChop <PyChop>` has been updated to calculate rep-rate multiplication for MAPS and MARI. In addition, it now supports loading instrument parameters from a YAML file, and also plots the Q-E coverage.
- Workspaces used in MSlice are now not added to the MantidPlot window automatically, and can be added using the ``Save To MantidPlot`` button.
- Improved default limits for 2D plots of large datasets in MSlice.

Algorithms
----------

New
###

- The *EPPWorkspace* input property has been removed from :ref:`DirectILLCollectData <algm-DirectILLCollectData>`.

Improvements
############

- :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>`:
    - it is now possible to set the thresholds for elastic peak and noisy background diagnostics in the IPFs
        - ILL's IN6 now sets its own default ``PeakDiagnosticsLowThreshold``
    - a hard mask is applied over the beamstop region of IN5
    - user masked detectors are not included in the report anymore
- :ref:`DirectILLReduction <algm-DirectILLReduction>`:
    - all output workspaces are now converted to distributions, i.e. the histograms are divided by the bin width.
    - The default :math:`Q` binning has been revised.

Bugfixes
########

- Fixed a crash in :ref:`SofQW <algm-SofQW>`, :ref:`SofQWCentre <algm-SofQWCentre>`, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and :ref:`SofQWPolygon <algm-SofQWPolygon>` algorithms when they were supplied with energy or :math:`Q` binning params containing the bin width only.
- Fixed a failure in the wavelength interpolation of :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` which occurred under certain input property combinations.

Deprecated
##########

- :ref:`TOFTOFMergeRuns	 <algm-TOFTOFMergeRuns>` is deprecated in favour of MergeRuns.

Instrument Definitions
----------------------

- The source component of ILL's IN5 has been moved from :math:`z = -2` to :math:`z = -2.10945` meters and renamed to ``frame-overlap_chopper``.
- The source component of ILL's IN6 has been moved from :math:`z = -0.395` to :math:`z = -0.595` meters and renamed to ``suppressor_chopper``.
- ILL's IN4 and IN6 now validate the wavelengths and chopper speeds in :ref:`MergeRuns <algm-MergeRuns>`.
- New CNCS geometry and parameters for 2018B cycle
- ARCS and CNCS are configured for live data

Python
------

Improvements
############

- The plotting methods in the :ref:`directtools <Directtools Python module>` python module now support logarithmic scales.

:ref:`Release 3.13.0 <v3.13.0>`
