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
  perform monitor shifts without changing the selected tranmission spectrum.

Improved
########

- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame>` to correct TOF by path to individual pixel
- New CG2 definition file

- Multiple SANS Workflow Algorithms were converted into internal scripts.
  This removes the need for passing SANSState objects in unrolled histories.
  Additionally, it speeds up the reduction of each run by 30%.

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

:ref:`Release 4.2.0 <v4.2.0>`
