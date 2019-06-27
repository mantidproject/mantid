=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
---

- The new algorithm :ref:`algm-GravityCorrection` corrects for the gravity effects for reflectometry workspaces by moving counts to ideal detector and respective time-of-flight bin.

ISIS Reflectometry Interface
----------------------------

Bug fixes
#########

- Fixed an error about an unknown property value when starting the live data monitor from the reflectometry interface.
	
Algorithms
----------

Improvements
############

- The output workspaces of :ref:`algm-ReflectometrySliceEventWorkspace` now have names which describe the slice.
- An additional method to calculate background has been added to :ref:`algm-ReflectometryBackgroundSubtraction`.
- In ref:`ReflectometryISISLoadAndPreprocess` the TOF workspaces are now grouped together.

Bug fixes
#########

- Fixed a bug in :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` that resulted in slightly too small bins in the output workspace.

Removed
#######

- Version 1 of `FindReflectometryLines` has been removed. Use :ref:`FindReflectometryLines-v2 <algm-FindReflectometryLines>` instead.

:ref:`Release 4.1.0 <v4.1.0>`
