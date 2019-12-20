=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

* Fixed an uncaught exception when plotting logs on single spectrum workspaces in mantidworkbench
* Save the units for single value logs in :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`

Concepts
--------

Improvements
############

- Fixed a bug in :ref:`LoadNGEM <algm-LoadNGEM>` where precision was lost due to integer arithmetic.

Algorithms
----------

Data Objects
------------



Geometry
--------

Improvements
############

- Increased numerical accuracy when calculating the bounding box of mili-meter sized cylindrical detector pixels.



Python
------

:ref:`Release 4.3.0 <v4.3.0>`
