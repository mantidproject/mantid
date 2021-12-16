=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
------------

- In Indirect Data Analysis fitting tabs a button has been added that will unify the fit range for all spectra selected.
- The Bayes Fortran libraries can now be installed on OSX and Linux using the bayesfitting python package https://pypi.org/project/bayesfitting/0.1.0/. This can be installed with `python -m pip install bayesfitting`
  using the Mantid Python executable.
- VesuvioAnalysis now allows defining constraints for more than two masses using the ConstraintsProfile, see :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>`.

Improvements
------------

- The Indirect/Simulation/DensityOfStates interface (and corresponding
  SimulatedDensityOfStates algorithm) can import force constants data
  from CASTEP or Phonopy calculations, and sample an appropriate
  q-point mesh on-the-fly to create a phonon DOS. This feature
  requires the Euphonic library to be installed; as it is not
  currently included in the Mantid release, an installer is provided
  in the Script Repository.
- In indirect Data analysis the Elwin Tab has had its UI updated to be more user friendly.
- Based on existing options for AnalysisMode in the VesuvioAnalysis algorithm two new
  options were introduced to allow reduction and analysis of spectra in the TOF domain
  without automatically carrying out a Y space analysis afterwards.
- Updated documentation for VesuvioAnalysis

Bugfixes
--------

- Fixed a bug which prevented workspaces being loaded into Elwin.
- Fixed a bug which caused VesuvioAnalysis to crash when run with a single element.
- Contour workspaces are now saved when saving in Bayes stretch.
- The Abins algorithm can also import force constants data from CASTEP
  or Phonopy calculations, using the Euphonic library. (See above.)

:ref:`Release 6.3.0 <v6.3.0>`
