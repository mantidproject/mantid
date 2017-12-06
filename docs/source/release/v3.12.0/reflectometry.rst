=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.
    
- The *BraggAngle* property of :ref:`algm-LoadILLReflectometry` now works as expected: the detector
  will be rotated such that the reflected peak will be at twice *BraggAngle*.
- The new algorithm :ref:`algm-LoadILLPolarizationFactors` can load the polarization efficiency files used on D17 at ILL.
- Removed the 'DirectBeam' box from the settings tab of the ISIS Reflectometry interface.
- Removed the ``RegionOfDirectBeam`` property from :ref:`algm-ReflectometryReductionOne` and
:ref:`algm-ReflectometryReductionOneAuto`

:ref:`Release 3.12.0 <v3.12.0>`
