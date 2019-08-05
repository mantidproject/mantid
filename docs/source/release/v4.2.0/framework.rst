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
* ref:`MaskAngle <algm-MaskAngle>` has an additional option of ``Angle='InPlane'``

Data Objects
------------
* New methods :py:obj:`mantid.api.SpectrumInfo.azimuthal` and :py:obj:`mantid.geometry.DetectorInfo.azimuthal`  which returns the out-of-plane angle for a spectrum

Python
------

:ref:`Release 4.2.0 <v4.2.0>`
