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

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 6.3.0 <v6.3.0>`
