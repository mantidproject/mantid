========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms
----------


New Algorithms
##############

- Added a new algorithm to ILL's reduction workflow: :ref:`DirectILLTubeBackground <algm-DirectILLTubeBackground>` which can be used to calculate the time-independent backgrounds for instruments with PSD detectors such as IN5.

Interfaces
----------


New features
############

- Added the ability to save the results of the TOFTOF reduction as Ascii files.


Improvements
############

- Improved ``Save``-section of the TOFTOF reduction dialog.
- Behavior of the :ref:`LoadDNSLegacy <algm-LoadDNSLegacy>` for TOF data has been changed: the algorithm does not try to guess elastic channel any more, but asks for the user input.
- :ref:`LoadDNSSCD <algm-LoadDNSSCD>` has been improved to be able to load TOF data.
- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` now can handle merged MD workspaces.

:ref:`Release 3.14.0 <v3.14.0>`

