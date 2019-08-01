=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improved
########

- Instrument definition is improved for IN16B to have the physical detector on the correct side of the beam axis, and different analyser focus for single detectors.
- :ref:`LoadILLIndirect <algm-LoadILLIndirect>` is extended to support also the configurations with the first tube angle at 33.1 degrees.
- :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` now offers the possibility to enable or disable the detector grouping both for Doppler and BATS modes. By default the pixels will be grouped tube by tube as before.

:ref:`Release 4.2.0 <v4.2.0>`


Data Analysis Interface
------------------------

Improvements
############
- Improved the output plotting options for Elwin and Iqt so that it is now possible to choose
  a list of spectra for plotting. For example, entering 0-2,4,6-8 and clicking **Plot Spectra**
  will plot the workspace indices 0, 1, 2, 4, 6, 7 and 8. The same can be done for **Plot Tiled**
  on the Iqt tab.


Data Reduction Interface
------------------------

Improvements
############
- Improved the output plotting options so that it is now possible to choose a list of spectra
  for plotting. This is the same as described for the Elwin and Iqt tab in Data Analysis.
- Improved the output plotting options so that they now show the workspace currently selected.
  The options have also been made more consistent across the interface.


Data Corrections Interface
--------------------------

Improvements
############
- Improved the output plotting options so that it is now possible to choose a list of spectra
  for plotting.
- Improved the output plotting options so that they now show the workspace currently selected.
  The options have also been made more consistent across the interface.


Simulations Interface
---------------------

Improvements
############
- Improved the output plotting options so that it is now possible to choose a list of spectra
  for plotting for the relevant tabs.
- Improved the output plotting options so that they now show the workspace currently selected.
  The options have also been made more consistent across the interface.


Diffraction Interface
---------------------

Improvements
############
- Improved the output plotting options so that it is now possible to choose a list of spectra
  for plotting. It also possible to do a contour plot using **Plot Contour**.
- Improved the output plotting options so that they now show the workspace currently selected.