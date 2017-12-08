====================
Spectroscopy Changes
====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Direct Geometry
---------------

- New algorithm :ref:`HyspecScharpfCorrection <algm-HyspecScharpfCorrection-v1>` that can be used to calculate spin incoherent scattering from polarized neutron data
- TOFTOF data reduction GUI has been improved. In the new version it has options to delete intermediate workspaces, to replace NaNs in S(Q,W), to create diffractograms and to save the reduced data in NXSPE and NeXus format.
- :ref:`algm-MonitorEfficiencyCorUser` is not anymore restricted to TOFTOF instrument.

Indirect Geometry
-----------------

- New algorithm :ref:`BASISDiffraction <algm-BASISDiffraction-v1>` to determine the orientation of crystal samples for the BASIS beamline.

:ref:`Release 3.12.0 <v3.12.0>`
