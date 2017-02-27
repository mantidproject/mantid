==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

- :ref:`EnergyWindowScan <algm-EnergyWindowScan>` and :ref:`IndirectQuickRun <algm-IndirectQuickRun>` have been added
  to perform a quick run of *EnergyTransfer*, *Elwin* and optional *MSDFit*
- A new algorithm :ref:`NMoldyn4Interpolation <algm-NMoldyn4Interpolation>` which interpolates simulated data onto reference OSIRIS data
- A new algorithm :ref:`Abins <algm-Abins>`  which allows comparison of theoretical and experimental INS.
- A 'QuickRun' algorithm :ref:`SofQWMomentsScan <algm-SofQWMomentsScan>` that reduces data and runs through :ref:`SofQW <algm-SofQW>` and :ref:`SofQWMoments <algm-SofQWMoments>`,
   performs a Lorentzian fit and from that result calculates the diffusion coefficient.

Data Reduction
##############

- Q-values in :ref:`BASISReduction <algm-BASISReduction>` output are now point data so that their values display correctly when plotted
- :ref:`LoadILLIndirect-v2 <algm-LoadILLIndirect-v2>` now checks in the ``.nxs`` files which single detectors (SD) are enabled, and loads only those instead of all, while moving them to the correct angle read from the file.
- New :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` algorithm performs initial data reduction steps for IN16B instrument data at ILL.
- New :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` algorithm performs complete multiple file reduction for Quasi-Elastic Neutron Scattering (QENS) data from IN16B instrument at ILL.
- New :ref:`IndirectILLReductionFWS <algm-IndirectILLReductionFWS>` algorithm performs complete multiple file reduction for the elastic and inelastic fixed-window scan data from IN16B instrument at ILL.
- Deprecated :ref:`IndirectILLReduction <algm-IndirectILLReduction>` and :ref:`ILLIN16BCalibration <algm-ILLIN16BCalibration>` algorithms.
- When plotting *ConvFit* results "Two Lorentzians" will produce plots for both lorentzians

Data Analysis
#############

- :ref:`TeixeiraWaterSQE <func-TeixeiraWaterSQE>` models translation of water-like molecules (jump diffusion).
- :ref:`GetQsInQENSData <algm-GetQsInQENSData>` Extracts or computes Q values from a MatrixWorkspace.
- *Elwin*  and 'QuickRun' algorithms now uses sample environment units found in sample logs
- :ref:`IsoRotDiff <func-IsoRotDiff>` models isotropic rotational diffusion of a particle
  tethered to the origin at a constant distance.

Corrections
###########

CalculatePaalmanPings
~~~~~~~~~~~~~~~~~~~~~

- Option to calculate number density from mass density

Absorption
~~~~~~~~~~

- Option to calculate number density from mass density
- Absorption geometry has been updated to use the :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` method

Tools
#####

Transmission
~~~~~~~~~~~~

- Option to calculate number density from mass density

Diffraction
###########

- Add option for normalisation by vanadium to spectroscopy mode. Divides the sample by vanadium after container subtraction.

Vesuvio
#######

- Run numbers can now be input as a range in :ref:`LoadVesuvio <algm-LoadVesuvio>` and :ref:`VesuvioDiffractionReduction <algm-VesuvioDiffractionReduction>`
- Position of monitors has been updated

Improvements
------------

- Data saved in an ASCII format using the *EnergyTransfer* interface can be re-loaded into Mantid
- TOSCA instrument definition file has been updated
- When plotting from interfaces the plots now display error bars as standard
- *I(Q, t)Fit* now uses the ExpDecay and StretchedExp functions already in Mantid

Bugfixes
--------

- Clicking 'Save' without creating a res file in *ISISCalibration* no longer causes an error
- Fixed issue when trying to plot multiple spectra from Indirect interfaces
- The plot options for *I(Q,t)Fit* had 'beta' displayed twice and the options did not plot the respective parameter
- *Jumpfit* creates a HWHM workspace in order to avoid the original workspace data being halved whenever loaded

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
