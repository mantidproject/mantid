==========================
Indirect Inelastic Changes
==========================

.. contents:: Table of Contents
   :local:

New features
------------

Algorithms
##########

Data Reduction
##############

- Q-values in :ref:`BASISReduction <algm-BASISReduction>` output are now point data so that their values display correctly when plotted

Data Analysis
#############

- :ref:`TeixeiraWaterSQE <func-TeixeiraWaterSQE>` models translation of water-like molecules (jump diffusion).


Corrections
###########

CalculatePaalmanPings
~~~~~~~~~~~~~~~~~~~~~

- Option to calculate number density from mass density

Absorption
~~~~~~~~~~~

- Option to calculate number density from mass density

Tools
#####

Transmission
~~~~~~~~~~~~

- Option to calculate number density from mass density

- :ref:`IsoRotDiff <func-IsoRotDiff>` models isotropic rotational diffusion of a particle
  tethered to the origin at a constant distance.


Improvements
------------
 - Data saved in an ASCII format using the *EnergyTransfer* interface can be re-loaded into Mantid

Bugfixes
--------
- Clicking 'Save' without creating a res file in *ISISCalibration* no longer causes an error


`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Indirect+Inelastic%22>`_
