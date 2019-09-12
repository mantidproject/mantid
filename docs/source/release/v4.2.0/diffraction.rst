===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------

Improvements
############

- The HRPD scripts now mask out the Bragg peaks from the Vanadium.
- The file-naming scheme for ISIS powder is now controlled by a string template
- The file-naming of output on HRPD as been updated to closely match old script outputs
- Geometry definition for LLB 5C1

Bug Fixes
#########

- The values used to mask the prompt pulse on HRPD have been fixed.
- :ref:`AlignAndFocusPowderFromFiles <AlignAndFocusPowderFromFiles-v1>` will reload the instrument if logs are skipped

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

Improvements
############

- :ref:`SaveHKL <algm-SaveHKL>` now saves the tbar and transmission values for shapes and materials provided by :ref:`SetSample <algm-SetSample>`.

Imaging
-------

:ref:`Release 4.2.0 <v4.2.0>`
