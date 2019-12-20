=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Improvements
############

- Fixed a bug in :ref:`LoadNGEM <algm-LoadNGEM>` where precision was lost due to integer arithmetic.

Algorithms
----------

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` can now save table workspaces, and :ref:`LoadAscii <algm-LoadAscii>` can load them again.


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
