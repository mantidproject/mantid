=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

Direct Geometry
---------------

New Algorithms
##############

- :ref:`ApplyDetailedBalanceMD <algm-ApplyDetailedBalanceMD>` to apply detailed balance to MDEvents
- :ref:`DgsScatteredTransmissionCorrectionMD <algm-DgsScatteredTransmissionCorrectionMD>` weights the intensity of each detected event according to its final energy.

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


CrystalField
------------

Improvements
############
- Added documentation and warning messages in the :ref:`Crystal Field Python Interface` related to IntensityScaling

BugFixes
########
- A bug has been fixed in the plot methods for CrystalField and CrystalFieldMultiSite
- Fixed a bug in the :ref:`Crystal Field Python Interface` which prevented the application of IntensityScaling factors
- Peaks are (re)set upon rebuilding the single spectrum function as a multi-spectrum function
  due to the physical properties. This re-setting peaks is needed to maintain the intended ties.

:ref:`Release 6.2.0 <v6.2.0>`
