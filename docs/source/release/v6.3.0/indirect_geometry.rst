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

- In Indirect Data Analysis fitting tabs a button has been added that will unify the fit range for all spectra selected.

Improvements
------------

- The Indirect/Simulation/DensityOfStates interface (and corresponding
  SimulatedDensityOfStates algorithm) can import force constants data
  from CASTEP or Phonopy calculations, and sample an appropriate
  q-point mesh on-the-fly to create a phonon DOS. This feature
  requires the Euphonic library to be installed; as it is not
  currently included in the Mantid release, an installer is provided
  in the Script Repository.

:ref:`Release 6.3.0 <v6.3.0>`
