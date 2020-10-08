============
SANS Changes
============

.. contents:: Table of Contents
   :local:

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
 - Powder diffraction support (instruments D2B and D20) has been added to DrILL. See :ref:`DrILL documentation <DrILL-ref>`
   for more information.


:ref:`Release 6.0.0 <v6.0.0>`
