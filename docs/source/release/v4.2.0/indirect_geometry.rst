=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improved
########

- Instrument definition is improved for IN16B to have the physical detector on the correct side of the beam axis, and different analyser focus for single detectors.
- :ref:`LoadILLIndirect <algm-LoadILLIndirect>` is extended to support also the configurations with the first tube angle at 33.1 degrees.
- :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` now offers the possibility to enable or disable the detector grouping both for Doppler and BATS modes. By default the pixels will be grouped tube by tube as before.

:ref:`Release 4.2.0 <v4.2.0>`
