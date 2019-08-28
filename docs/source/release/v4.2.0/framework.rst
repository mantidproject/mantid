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

Algorithms
----------
* :ref:`MaskAngle <algm-MaskAngle>` has an additional option of ``Angle='InPlane'``
* :ref:`FitIncidentSpectrum <algm-FitIncidentSpectrum>` will fit a curve to an incident spectrum returning the curve and it's first derivative.
* Whitespace is now ignored anywhere in the string when setting the Filename parameter in :ref:`Load <algm-Load>`.

Data Objects
------------

Python
------

Bug Fixes
---------
* ref:`LoadNexusMonitors <algm-LoadNexusMonitors>` bug fix for user provided top-level NXentry name 

:ref:`Release 4.2.0 <v4.2.0>`

