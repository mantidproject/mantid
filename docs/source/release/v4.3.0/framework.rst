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
- Prevent units that are not suitable for :ref:`ConvertUnits <algm-ConvertUnits>` being entered as the target unit.

Algorithms
----------

Data Objects
------------

Python
------

:ref:`Release 4.3.0 <v4.3.0>`
