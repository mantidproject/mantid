=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

Direct Geometry
---------------

New Algorithms
##############

- :ref:`LoadDNSEvent <algm-LoadDNSEvent>` loads data from DNS PSD detector into EventWorkspace

Improvements
############
- The :ref:`Crystal Field Python interface <Crystal Field Python Interface>` has two new fitting functions, ``two_step_fit`` and ``two_step_fit_sc``, alternating optimization over field parameters and peak parameters. One function is based on the Mantid fitting for both parts, the other uses ``scipy.optimize.minimize`` for the field parameters.
- The default value of monitor peak width multiplier (``MonitorPeakWidthInSigmas``) has been changed from 3 to 7 in :ref:`DirectILLCollectData <algm-DirectILLCollectData>` .
- The :ref:`PyChop <PyChop>` GUI now has the ability to handle multiple independently phased choppers.

Bugfixes
########
- Updated instrument created by :ref:`LoadNXSPE <algm-LoadNXSPE>` is viewable in instrument 3D view.
- Fixed a bug that caused empty plot windows and crashes when running scripts generated from plot windows in MSlice.

:ref:`Release 6.3.0 <v6.3.0>`
