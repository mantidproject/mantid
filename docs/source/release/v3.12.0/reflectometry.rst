=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.
    
    
ISIS Reflectometry Interface
----------------------------

New features
############

Improvements
############

- Menu items and toolbar buttons are now enabled/disabled when appropriate, e.g. to prevent table modification during processing. Directly editing table rows is also disabled during processing.
- Removed the 'DirectBeam' box from the settings tab of the ISIS Reflectometry interface because this is not used.

Bug fixes
#########


Algorithms
----------
    
New features
############

- The new algorithm :ref:`algm-LoadILLPolarizationFactors` can load the polarization efficiency files used on D17 at ILL.
- The new algorithm :ref:`algm-MRInspectData` takes in raw event data and determines reduction parameters.
- The ISIS Reflectometry interface now has a checkbox 'CorrectDetectors' which maps to the corresponding property in :ref:`algm-ReflectometryReductionOneAuto`.

Improvements
############

- Removed the ``RegionOfDirectBeam`` property from :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` because this is not used.

Bug fixes
#########

- The *BraggAngle* property of :ref:`algm-LoadILLReflectometry` now works as expected: the detector will be rotated such that the reflected peak will be at twice *BraggAngle*.


:ref:`Release 3.12.0 <v3.12.0>`
