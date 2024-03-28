=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- The Bayes Fortran libraries have been moved to a separate ``quasielasticbayes`` package.
  This allows those on non-Windows platforms to install them with  ``pip``: ``python -m pip install --user quasielasticbayes``.
- In :ref:`Inelastic Data Analysis <interface-inelastic-qens-fitting>` fitting tabs, a button has been added that will unify the fit range for all spectra selected.
- :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` now allows defining constraints for more than two masses using the ``ConstraintsProfile`` .

Improvements
------------
- Based on existing options for ``AnalysisMode`` in the :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` algorithm, two new options were introduced to allow reduction and analysis of spectra in the TOF domain
  without automatically carrying out a Y space analysis afterwards.
- Added a validation check to the :ref:`Apply Absorption Corrections Tab<apply_absorp_correct>` in the :ref:`Corrections GUI<interface-inelastic-corrections>` that makes sure the sample workspace and corrections workspaces all have the same number of histograms.
- The Indirect Simulation :ref:`DensityOfStates interface <DensityOfStates_Interface>` (and corresponding :ref:`SimulatedDensityOfStates <algm-SimulatedDensityOfStates>` algorithm) can import force constants data
  from CASTEP or Phonopy calculations, then sample an appropriate q-point mesh on-the-fly to create a phonon DOS. This feature requires the Euphonic library to be installed. This library is
  included in the Windows package, but for other platforms an installer is provided in the Script Repository.
- In Inelastic Data Analysis the :ref:`Elwin Tab <elwin>` has had its UI updated to be more user friendly.
- :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` excludes back scattering spectra for now to avoid problems with the analysis.
- Updated documentation for :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>`.

Bugfixes
--------
- The :ref:`Abins Algorithm <algm-Abins>` can also import force constants data from CASTEP or Phonopy calculations, using the Euphonic library. (See above.)
- Contour workspaces are now saved when saving in ``Bayes stretch``.
- Fixed a bug which prevented workspaces being loaded into the :ref:`Elwin Tab <elwin>`.
- Fixed a bug that caused :ref:`ISIS Calibration <interface-indirect-isis-calibration>` to ignore ``ScaleFactor`` if set to 1.0.
- Fixed a bug that caused :ref:`ISIS Calibration <interface-indirect-isis-calibration>` to not set valid defaults for certain runs.
- Fixed a bug in the :ref:`Symmetrise<algm-Symmetrise>` algorithm where workspaces in different bin widths would sometimes cause exceptions.
- Fixed a bug which caused :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` to crash when run with a single element.


:ref:`Release 6.3.0 <v6.3.0>`
