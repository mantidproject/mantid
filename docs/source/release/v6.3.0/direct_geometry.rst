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

- The default value of monitor peak width multiplier (MonitorPeakWidthInSigmas) has been changed from 3 to 7 in :ref:`DirectILLCollectData <algm-DirectILLCollectData>`
- The :ref:`PyChop <PyChop>` GUI now has the ability to handle multiple independently phased choppers
- The Crystal Field Python interface has two new fitting functions, `two_step_fit` and `two_step_fit_sc`, alternating optimization over field parameters and peak parameters. One function is based on the Mantid fitting for both parts, the other uses scipy.optimize.minimize for the field parameters.

Bugfixes
########

- Update instrument created by :ref:`LoadNXSPE <algm-LoadNXSPE>` to be viewable in instrument 3D view.
- Fixed a bug that caused empty plot windows and crashes when running scripts generated from plot windows.

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 6.3.0 <v6.3.0>`
