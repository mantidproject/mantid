============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Bugs fixes
----------

- Fix a bug that made it impossible to process flux in SANSILLAutoprocess.

Improvements
------------

- With SANSILLAutoProcess, the detector distance, the collimation position and the wavelength are appended to the names of the output workspaces (values are taken from the sample logs).
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has new property:
  `StitchReferenceIndex` to denote the index of ws that should be a reference
  for scaling during stitching

:ref:`Release 6.1.0 <v6.1.0>`
