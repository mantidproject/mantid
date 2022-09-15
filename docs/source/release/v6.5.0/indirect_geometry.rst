=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- A new set of scripts has been produced to provide a method for processing OSIRIS data. :ref:`isis-powder-diffraction-osiris-ref`
- Three new fitting functions: FickDiffusionSQE, ChudleyElliotSQE, and HallRossSQE have been made and added to ConvFit
- The I(q,t) tab in Indirect Data Analysis can now be ran with direct data
- Updated :ref:`SimpleShapeDiscusInelastic <algm-SimpleShapeDiscusInelastic>` workflow algorithm to add support for containers in line with the enhancements made to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>`


Bugfixes
--------
- Fixed a bug where clicking 'Run' on the :ref:`Apply Absorption Corrections Tab<indirect_apply_absorp_correct>` in the :ref:`Corrections GUI<interface-indirect-corrections>` with no Sample or Corrections would close mantid.
- Fixed a bug where if the Corrections Workspace name entered on the :ref:`Apply Absorption Corrections Tab<indirect_apply_absorp_correct>` does not match an exisiting workspace Mantid would close.
- Fixed a bug in Indirect Data Reduction where the spectra in the detector table started at 0. The spectra now start at 1.


Algorithms
----------

New features
############


Bugfixes
############


:ref:`Release 6.5.0 <v6.5.0>`