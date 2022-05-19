=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:


New Features
------------

- In Indirect Data analysis the button for fitting the currently plotted spectra has been renamed and rescaled to fit the interface.
- :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` and :ref:`IndirectILLReductionFWS <algm-IndirectILLReductionFWS>` now offer a possibility to switch off the detector grouping. An option has been added to ISISEnergyTransfer that will group outputs by the position of the sample changer.


Bugfixes
--------

- A bug has been fixed in Indirect Data Analysis where the first value would be ignored when a fit was performed.
- An error is fixed in Abins. This relates to the assignment of "Forward" and "Backward" detector settings for TOSCA, which werereversed from their conventional definition. The default behaviour has not changed: results will only be affected for calculations which explicitly selected the "Forward (TOSCA)" or "Backward (TOSCA)" options.
- A bug has been fixes in SofQWMoments that prevented it from working with OSIRIS data.
- A bug has been fixed in the CASTEP .phonon file import for the Indirect/Simulation/DensityOfStates interface and SimulatedDensityOfStates algorithm. The bug was causing some q-points to be misidentified as duplicate Gamma-points and removed.

Algorithms
----------

New Features
############

- Add new workflow algorithm called :ref:`SimpleShapeDiscusInelastic <algm-SimpleShapeDiscusInelastic>` that calls :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` with options for some simple shape types

Bugfixes
########

- The Abins algorithm has been reworked to account for q-dependence correctly when forming 3+
  phonon events by self-convolution. This changes the results obtained when using the "Autoconvolution"
  option, which now results in the expected high-frequency "wing".

.. image::  ../../images/abins-6.4-high-order.png
            :align: center


:ref:`Release 6.4.0 <v6.4.0>`