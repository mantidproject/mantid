=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 4.1.0 <v4.1.0>`

Instrument Definitions
----------------------

The IDF of IN16B has been rewritten to properly model the neutronic positions of the pixels as reflections wrt spherical analysers.
This is needed for the new inverted time-of-flight mode (BATS), but also improves the model for the standard doppler mode.
Particularly, this will allow to calculate correct absorption corrections before grouping the pixels tube by tube.
With the new IDF the flight-paths inside the sample will properly take into account the off-plane angle, which was not the case with the previous IDF.
This change has a retroactive effect, but for reductions where the detectors were grouped, it should not produce different results.

.. figure:: ../../images/IN16B-R.png
   :align: left
   :scale: 49%

.. figure:: ../../images/IN16B-N.png
   :align: right
   :scale: 50%

Algorithms
----------

Improvements
############

- :ref:`ModeratorTzeroLinear <algm-ModeratorTzeroLinear>` permits now passing parameter values as input properties.
- :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>` resolves between run with old and new DAS.
- :ref:`BASISPowderDiffraction <algm-BASISPowderDiffraction>` permits now flux normalization by proton charge and run duration.
- :ref:`BASISReduction <algm-BASISReduction>` permits now flux normalization by proton charge and run duration.
- :ref:`BASISReduction <algm-BASISReduction>` permits now retaining events only within a time window.
- :ref:`BASISReduction <algm-BASISReduction>` can output now the powder diffraction spectra.
- :ref:`BASISCrystalDiffraction <algm-BASISCrystalDiffraction>` resolves between run with old and new DAS.


Data Analysis Interface
-----------------------

Improvements
############
- Improved the output options of MSD Fit, Iqt Fit, Conv Fit and F(Q)Fit so that Chi_squared can now be plotted.
- Improved the I(Q, t) tab by adding more validation checks for the input data.
- Improved the Fit and Difference plots in MSD Fit, Iqt Fit, Conv Fit and F(Q)Fit. It is now possible to adjust their
  relative sizes by dragging a 'handle' between the plots.
- Improved the I(Q, t) tab by allowing an asymmetric energy range by unticking 'Symmetric Energy Range'.
- Improved the I(Q, t) tab by adding the ability to change the plotted spectrum using the 'Spectrum' spin box.

Bug Fixes
#########
- Fixed an error caused by loading a Sample into ConvFit which does not have a resolution parameter for the analyser.
- Fixed a crash caused by changing the Preview Spectrum on Elwin after clicking Run.
- Fixed a bug where the loaded workspace in Data Analysis doesn't update after being changed on a different
  interface.


Data Corrections Interface
--------------------------

Improvements
############
- Improved the setting of sample and container neutron information by allowing the entry of cross sections as an
  alternative to a chemical formula. The cross sections can be entered in the Calculate Paalman Pings tab and
  Calculate Monte Carlo Absorption tab. This uses the :ref:`SetSampleMaterial <algm-SetSampleMaterial>` algorithm.

Bug Fixes
#########
- Fixed a bug where the output plots on the Calculate Paalman Pings and Calculate Monte Carlo Absorption tabs had
  the wrong axis labels and units.


Data Reduction Interface
------------------------

Improvements
############
- Added an option called *Group Output* to group the output files from a reduction on ISISEnergyTransfer.

Bug Fixes
#########
- Fixed a bug in the :ref:`Integration <algm-Integration>` algorithm causing the Moments tab to crash.
- Fixed an unexpected error when opening the Data Reduction interface with an unrelated facility selected.
