=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.
	
Algorithms
----------

New
###

Improvements
############

- An additional method to calculate background has been added to :ref:`algm-ReflectometryBackgroundSubtraction`.

Bug fixes
#########


Algorithms
----------

Bug fixes
#########

- Fixed a bug in :ref:`ReflectometrySumInQ <algm-ReflectometrySumInQ>` that resulted in slightly too small bins in the output workspace.

Removed
#######

- Version 1 of `FindReflectometryLines` has been removed. Use :ref:`FindReflectometryLines-v2 <algm-FindReflectometryLines>` instead.

:ref:`Release 4.1.0 <v4.1.0>`
