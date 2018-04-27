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

Bug fixes
#########

- Fixed a crash in :ref:`SofQW <algm-SofQW>`, :ref:`SofQWCentre <algm-SofQWCentre>`, :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` and :ref:`SofQWPolygon <algm-SofQWPolygon>` algorithms when they were supplied with energy or :math:`Q` binning params containing the bin width only.

Instrument Definitions
----------------------


- The source component of ILL's IN5 has been moved from :math:`z = -2` to :math:`z = -2.10945` meters and renamed to ``frame-overlap_chopper``.
- The source component of ILL's IN6 has been moved from :math:`z = -0.395` to :math:`z = -0.595` meters and renamed to ``suppressor_chopper``.

:ref:`Release 3.13.0 <v3.13.0>`

