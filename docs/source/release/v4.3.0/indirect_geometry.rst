=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
###

  - Simultaneous fitting has been added as an option on the convolutional fitting tab of data analysis.

Improvements
############

- In Abins the instrumental broadening implementation has been overhauled.

  - Output spectra from Abins should now be smooth and free of artefacts.
  - A fast approximate scheme is implemented and automatically applied
    when appropriate for simulations of TOSCA spectra. Alternative
    broadening implementations may be selected through the
    AbinsParameters system, including slow reference methods.
  - Spectra now follow the broader Mantid convention for histograms:
    values correspond to frequencies at the mid-points of the
    histogram bins. (Previously the values would correspond to one
    bin-edge.)

BugFixes
########

- The Abins file parser no longer fails to read data from non-periodic vibration calculations performed with CRYSTAL17.
- :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>` will now run also for fixed window scan reduced data and will not crash on workspace groups.
- Fixed a bug crashing the ``Indirect ILL Data Reduction GUI`` when Run is clicked.

:ref:`Release 4.3.0 <v4.3.0>`
