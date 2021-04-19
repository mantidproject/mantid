============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Bugfixes
--------

- Fix a bug that made it impossible to process flux in SANSILLAutoprocess.
- On D16 using :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, now use the correct monitor for normalization, fix a bug where processing transmission would yield undefined values at 90 degrees when using ThetaDependent correction, and improve the q binning used.

Improvements
------------

- With SANSILLAutoProcess, the detector distance, the collimation position and the wavelength are appended to the names of the output workspaces (values are taken from the sample logs).
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has new property: `StitchReferenceIndex` to denote the index of ws that should be a reference
  for scaling during stitching
- :ref:`SANSILLReduction <algm-SANSILLReduction>` has a new property `SolventInputWorkspace`, to provide reduced solvent data to be subtracted from the sample data.
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has a new property `SolventFiles`, to communicate with :ref:`SANSILLReduction <algm-SANSILLReduction>` the file names of the reduced solvent data.


Algorithms and instruments
--------------------------


:ref:`Release 6.1.0 <v6.1.0>`
