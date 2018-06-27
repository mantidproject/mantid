========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------


New features
############

- The *EPPWorkspace* input property has been removed from :ref:`DirectILLCollectData <algm-DirectILLCollectData>`.

Improvements
############

- :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>`:
    - a hard mask is applied over the beamstop region of IN5
    - user masked detectors are not included in the report anymore
- :ref:`DirectILLReduction <algm-DirectILLReduction>`:
    - all output workspaces are now converted to distributions, i.e. the histograms are divided by the bin width.
    - The default :math:`Q` binning has been revised.

Bug fixes
#########

- Fixed a crash in :ref:`SofQW <algm-SofQW>`, :ref:`SofQWCentre <algm-SofQWCentre>`, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and :ref:`SofQWPolygon <algm-SofQWPolygon>` algorithms when they were supplied with energy or :math:`Q` binning params containing the bin width only.
- Fixed a failure in the wavelength interpolation of :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` which occurred under certain input property combinations.

Instrument Definitions
----------------------


- The source component of ILL's IN5 has been moved from :math:`z = -2` to :math:`z = -2.10945` meters and renamed to ``frame-overlap_chopper``.
- The source component of ILL's IN6 has been moved from :math:`z = -0.395` to :math:`z = -0.595` meters and renamed to ``suppressor_chopper``.
- New CNCS geometry and parameters for 2018B cycle
- ARCS and CNCS are configured for live data

Python
------

- The plotting methods in the :ref:`directtools <Directtools Python module>` python module now support logarithmic scales.

:ref:`Release 3.13.0 <v3.13.0>`
