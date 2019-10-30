============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
###
- Support for shifting both monitor 4 and 5 on Zoom including a new setting in the 
  ISIS SANS GUI. A new user file command has also been added to
  perform monitor shifts without changing the selected transmission spectrum.
- New :ref:`HFIRSANS2Wavelength <algm-HFIRSANS2Wavelength-v1>` algorithm to "convert" CG2 event files
  to histograms in wavelength.

Improved
########

- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame-v1>` to correct TOF by path to individual pixel
- New CG2 definition file
- A bug causing large batch files (1000+ runs) to take minutes to load into the
  ISIS SANS GUI has been fixed. Large batch files will now load within seconds.
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection-v1>` now can be supplied any transmission workspace that is supported by :ref:`Divide <algm-Divide-v1>` .

- Multiple SANS Workflow Algorithms were converted into internal scripts.
  This removes the need for passing SANSState objects in unrolled histories.
  Additionally, it speeds up the reduction of each run by ~35%.

Removed
#######

The following SANS Workflow algorithms were removed:
- SANSCalculateTransmission
- SANSCreateAdjustment
- SANSCrop
- SANSConvertToQ
- SANSConvertToWavelength
- SANSMaskWorkspace
- SANSMove
- SANSNormalizeToMonitor
- SANSScale
- SANSSliceEvent

:ref:`Release 4.2.0 <v4.2.0>`
