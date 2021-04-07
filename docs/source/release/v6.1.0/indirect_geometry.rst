=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
############
- Three fitting functions `IsoRotDiff`, `DiffSphere` and `DiffRotDiscreteCircle` have been made available in the fitting browser
- `DiffSphere` and `DiffRotDiscreteCircle` have been added to the function options in Indirect Data Analysis ConvFit.
- The Abins Algorithm has an additional "setting" option which may be
  used to select between configurations of a given instrument. (For
  TOSCA this is a choice of forward/back detector banks, for Lagrange
  this is a choice between monochromator crystals.)
- Support has been added to Abins for the ILL-Lagrange
  instrument. As Lagrange collects inelastically scattered neutrons
  over a wide solid angle, the spectrum is computed at several angles
  and averaged. Resolution functions are applied depending on the
  monochromator setting.

Improvements
############
- In Indirect Data Analysis F(Q) fit the default fitting function remains None when switching to EISF.

:ref:`Release 6.1.0 <v6.1.0>`
