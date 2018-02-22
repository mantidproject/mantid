====================
Spectroscopy Changes
====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

- The algorithms :ref:`algm-SofQWCentre`, :ref:`algm-SofQWPolygon` and :ref:`algm-SofQWNormalisedPolygon`, which rebin an inelastic workspace (has a `DeltaE` axis) from spectrum numbers (angle) to `MomentumTransfer` may now rebin the energy (`DeltaE`) axis as well as the :math:`|Q|` (`MomentumTransfer`) axes.
- :ref:`algm-SofQWNormalisedPolygon` now has uses a faster method for calculating the polygon intersections.
- The crystal field computation and fitting engine is now feature complete. It can now handle multi-site computation and simultaneous fitting of inelastic spectra and physical properties dataset. See the :ref:`Crystal Field Python Interface` help page for details, and `<http://www.mantidproject.org/Crystal_Field_Examples>`_ for examples of use.

Direct Geometry
---------------

- New algorithm :ref:`HyspecScharpfCorrection <algm-HyspecScharpfCorrection-v1>` that can be used to calculate spin incoherent scattering from polarized neutron data
- TOFTOF data reduction GUI has been improved. In the new version it has options to delete intermediate workspaces, to replace NaNs in S(Q,W), to create diffractograms and to save the reduced data in NXSPE and NeXus format.
- :ref:`algm-MonitorEfficiencyCorUser` is not anymore restricted to TOFTOF instrument.

Indirect Geometry
-----------------

- New algorithm :ref:`BASISDiffraction <algm-BASISDiffraction-v1>` to determine the orientation of crystal samples for the BASIS beamline.

:ref:`Release 3.12.0 <v3.12.0>`
