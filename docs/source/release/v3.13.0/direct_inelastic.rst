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


Improvements
############

- :ref:`algm-DirectILLDiagnostics` doesn't report the detectors masked by user in the *OutputReport* string anymore.

Bug fixes
#########

- Fixed a crash in :ref:`SofQW <algm-SofQW>`, :ref:`SofQWCentre <algm-SofQWCentre>`, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and :ref:`SofQWPolygon <algm-SofQWPolygon>` algorithms when they were supplied with energy or :math:`Q` binning params containing the bin width only.

:ref:`Release 3.13.0 <v3.13.0>`

