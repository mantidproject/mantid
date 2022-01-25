=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- In :ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>` fitting tabs, a button has been added that will unify the fit range for all spectra selected.
- :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` now allows defining constraints for more than two masses using the ``ConstraintsProfile`` .

Improvements
------------
- Based on existing options for ``AnalysisMode`` in the :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` algorithm, two new options were introduced to allow reduction and analysis of spectra in the TOF domain
  without automatically carrying out a Y space analysis afterwards.
- The Indirect Simulation :ref:`DensityOfStates interface <DensityOfStates_Interface>` (and corresponding :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` algorithm) can import force constants data
  from CASTEP or Phonopy calculations, then sample an appropriate q-point mesh on-the-fly to create a phonon DOS. This feature requires the Euphonic library to be installed. This library is
  included in the Windows package, but for other platforms an installer is provided in the Script Repository.
- In Inelastic Data Analysis the :ref:`Elwin Tab <Elwin-iqt-ref>` has had its UI updated to be more user friendly.
- Updated documentation for :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>`.

Bugfixes
--------
- The :ref:`Abins Algorithm <algm-Abins>` can also import force constants data from CASTEP or Phonopy calculations, using the Euphonic library. (See above.)
- Contour workspaces are now saved when saving in ``Bayes stretch``.
- Fixed a bug which prevented workspaces being loaded into the :ref:`Elwin Tab <Elwin-iqt-ref>` .
- Fixed a bug which caused :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` to crash when run with a single element.

:ref:`Release 6.3.0 <v6.3.0>`
