============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Improvements
############

- Add support for D11, D16, D22 and D33 in the :ref:`MaskBTP <algm-MaskBTP>` algorithm.

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

Improvements
############

 - In :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, the beam radius can be different for each distance.
   A new parameter, TransmissionBeamRadius, has been added to set the beam radius for transmission experiments.
   The default value of all beam radii is now 0.1m.
 - With :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, if sample thickness is set to -1, the algorithm will try to get it
   from the nexus file.
 - With :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, the output workspace will get its title from the nexus file.

Bugfixes
########

- Detector IDs are no longer copied during a 2D reduction. This also resolves
  a bug where the first two spectra were marked as monitors and would not appear
  in a colour fill plot on Workbench.

:ref:`Release 6.0.0 <v6.0.0>`
