=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:


Improvements
############

- Instrument definition is improved for IN16B to have the physical detector on the correct side of the beam axis, and different analyser focus for single detectors.
- Able to choose a list of spectra for plotting for all Indirect Interfaces. For example, under Data Analysis>Elwin, entering 0-2,4,6-8 and clicking **Plot Spectra** will plot the workspace indices 0, 1, 2, 4, 6, 7 and 8. The same can be done for **Plot Tiled** on the Iqt tab. It is  also possible to produce a contour plot using **Plot Contour** on the Diffraction Interface.
- The Plotting Options have also been made more consistent across the interface.

.. figure:: ../../images/Indirect_Data_Analysis_IqtFit.PNG
  :class: screenshot
  :align: right
  :figwidth: 60%
  :alt: The Indirect Data Analysis GUI in the Workbench.

Workbench
-------------

- The Indirect Bayes GUI has been added to the Workbench.
- ``*``**The Indirect Data Analysis GUI has been added to the Workbench**``*``

Algorithms
----------

- :ref:`IndirectQuickRun <algm-IndirectQuickRun>` and :ref:`IndirectSampleChanger <algm-IndirectSampleChanger>` have been
  extended to allow them to perform a Width Fit. This utilizes the new algorithm :ref:`IndirectTwoPeakFit <algm-IndirectTwoPeakFit>`.
- :ref:`LoadILLIndirect <algm-LoadILLIndirect>` is extended to support also the configurations with the first tube angle at 33.1 degrees.
- :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` now offers the possibility to enable or disable the detector grouping both for Doppler and BATS modes. By default the pixels will be grouped tube by tube as before.
- :ref:`IndirectILLEnergyTransfer <algm-IndirectILLEnergyTransfer>` offers an option to discard the single detectors if they were not enabled in the measurement.
- :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` now checks input properties are valid.
- :ref:`IndirectILLReductionFWS <algm-IndirectILLReductionFWS>` will now integrate the inelastic peaks correctly, based on the peak positions in the monitor spectrum.


Simulations Interface
---------------------

- Improved the format of Abins HDF5 files to include readable set of advanced parameters. Values
  in the AbinsParameters module have been re-organised into a logical heirarchy; user scripts
  may need to be modified to match this.

BugFixes
########

- A bug has been fixed in :ref:`MatchPeaks <algm-MatchPeaks>` which was causing wrong alignment in :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` with unmirror option 7, when the peaks in the alignment run are too narrow to be fitted.
- Fixed a bug where mantid could crash when using the S(Q,w) tab with a workspace created by the Energy Transfer Tab.

``*`` See associated image ``*``

:ref:`Release 4.2.0 <v4.2.0>`
