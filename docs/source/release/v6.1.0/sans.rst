============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Bugfixes
----------

- Fix a bug that made it impossible to process flux in SANSILLAutoprocess.
- On D16 using :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, now use the correct monitor for normalization,
fix a bug regarding transmission processing at 90 degrees, and improve the q binning used.

Improvements
------------

- With SANSILLAutoProcess, the detector distance, the collimation position and the wavelength are appended to the names of the output workspaces (values are taken from the sample logs).

:ref:`Release 6.1.0 <v6.1.0>`
