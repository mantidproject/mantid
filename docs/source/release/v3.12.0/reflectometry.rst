=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

- :ref:`algm-LoadILLReflectometry` has received a few improvements.
    - Figaro NeXus files are now properly handled.
    - A new property, *BeamCentre* allows user to manually specify the beam position on the detector.
    - The *BeamPosition* property was renamed to *DirectBeamPosition* to better reflect its usage.
    - The *BraggAngle* property of :ref:`algm-LoadILLReflectometry` now works as expected: the detector will be rotated such that the reflected peak on the detector will be at twice *BraggAngle*.

:ref:`Release 3.12.0 <v3.12.0>`
