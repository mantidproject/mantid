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

Improved
########

- Option in :ref:`EQSANSCorrectFrame <algm-EQSANSCorrectFrame-v1>` to correct TOF by path to individual pixel
- New CG2 definition file
- :ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection-v1>` now can be supplied any transmission workspace that is supported by :ref:`Divide <algm-Divide-v1>` .

:ref:`Release 4.2.0 <v4.2.0>`
