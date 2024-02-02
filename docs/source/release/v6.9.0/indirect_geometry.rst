=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Removed unnecessary options from ISIS Energy Transfer tab and show only relevant options based on the selected instrument
-  The slice viewer can be opened instead of a contour in the external plot options of the Inelastic/Indirect Interfaces with a plot contour option.


Bugfixes
--------
- The :ref:`Indirect Diffraction <interface-indirect-diffraction>` interface will only use the 'bank' component for manual grouping if it exists, otherwise it will use 'diffraction'.
- In the :ref:`QECoverage <QE Coverage>` tool, fixed crash and replaced multiple pop-up windows by warnings when invalid input is passed to Ei and Emin (and S2 in the case of HYSPEC).
- Fixed a problem with integration and background limits not updated correctly from IPF in workspace input tab in :ref:`Elwin Tab <elwin>`
- Fixed a bug where ApplyAbsorptionCorrections would not load a *_Corrections* file if it only contained a single correction component.
- A bug where the ``Run`` and ``Output Options`` appeared squished on Indirect interfaces has been fixed.


Algorithms
----------

New features
############


Bugfixes
############
- An intermediate wrapper algorithm called ``ISISIndirectEnergyTransferWrapper`` algorithm has been removed. The :ref:`Data Reduction <interface-indirect-data-reduction>` interface now calls the :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>` algorithm directly.

:ref:`Release 6.9.0 <v6.9.0>`