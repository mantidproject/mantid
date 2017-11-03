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

- TOFTOF data reduction GUI has been improved. In the new version it has options to delete intermediate workspaces, to replace NaNs in S(Q,W), to create diffractograms and to save the reduced data in NXSPE and NeXus format.

Indirect Geometry
-----------------

- New algorithm :ref:`BASISDiffraction <algm-BASISDiffraction-v1>` to determine the orientation of crystal samples for the BASIS beamline.

:ref:`Release 3.12.0 <v3.12.0>`