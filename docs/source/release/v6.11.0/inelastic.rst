=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- Add `Load Workspace History` option to the settings dialog for Indirect/Inelastic interfaces.
- It is possible to access the Slice Viewer or 3D Plot from the OutputPlot widget of the in :ref:`Elwin Tab <elwin>` of  :ref:`Data Processor Interface <interface-inelastic-data-processor>` for output workspaces containing more than 1 histogram.
- Added an option to "Tie Amplitudes" for two Lorentzians in the Convolution tab of the :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>` interface.
- Added the ability to load `A0` fit parameter data into the `Function (Q)` tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface.
- Add `Output Composite Members` option to I(Q, t) tab in :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>`
- Rename tabs of :ref:`QENS Fitting <interface-inelastic-qens-fitting>` from MSD Fit, I(Q,t) Fit, ConvFit and F(Q) to MSD, I(Q,t), Convolution and Function(Q) respectively. Add tooltip.
- Add option to not delete subtracted workspaces when adding new data in the :ref:`Container Subtraction <container-subtraction>` tab of the Corrections interface.


Bugfixes
--------
- Fixed an 'index out of range' error in the :ref:`BayesQuasi <algm-BayesQuasi>` algorithm when using a sample with a Numeric Axis.
- Fix a freeze in :ref:`Elwin Tab <elwin>` of :ref:`Data Processor Interface <interface-inelastic-data-processor>` that happens when running the tab.
- Fix a crash in :ref:`Convolution <convfit>` :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>` interface when attempting to Fix all IsoDiffRot parameters from the EditLocalParameter dialog.
- fix comparison operation in ``ISISIndirectInelastic.OSIRISIqtAndIqtFitMulti`` system test by turning off uncertainty check, using absolute comparison, and adjusting the tolerance.
- If the ADS is cleared from workspaces that are used to run fits on an open :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>`, a warning message pops up when clicking on the `Run` button.
- Fixed a bug in the Monte Carlo error calculation on the I(Q, t) tab of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` where the first bin had an error of zero.
- Data unrestricted by suffix can be loaded on   :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` if the option is selected from Settings.
- It is possible to see the Spectrum number 0 on the widget plot of the selected preview workspace on :ref:`Elwin interface <elwin>`
- Changing the preview spectrum above the plot widget combo box plots the correct spectrum for the selected index  :ref:`Elwin interface <elwin>`
- Fix a bug in which sliders didn't respond to changes in `Emin` and `EMax` properties when changed from the property browser in the :ref:`Moments<inelastic-moments>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface.
- Fixed an invalid calculation of the EISF errors calculated on the Quasi tab of the :ref:`Inelastic Bayes fitting <interface-inelastic-bayes-fitting>` interface.
- Adding new data to the  :ref:`Elwin data table <elwin>` after clearing the Analysis Data Service no longer raises a *No data found* warning.
- Mantid no longer crashes when trying to plot a preview of the selected workspace on the :ref:`Elwin tab <elwin>` after that workspace has been deleted from the ADS.
- Fix a cutoff issue with "Symmetric Energy Range" label in :ref:`Iqt<iqt>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface.
- The dialog window for adding data in the  :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` no longer freezes when adding data.
- Fixed a crash on the Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface caused by attempting to load a WorkspaceGroup rather than the expected Workspace2D.
- Prevented a crash on the Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface caused by clicking `Run` before data has finished loading.
- Available fit functions in the `Function (Q)` tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface are updated according to the type of data (`EISF`, `A0` or `Width`) loaded on the table.


Algorithms
----------

New features
############
- Add support for "high-precision" eigenvectors from GAUSSIAN in Abins/Abins2D algorithms. These are activated in GAUSSIAN with the ``freq(HPModes)`` parameter. In previous Mantid versions the Abins parser would fail to read the resulting files; now the high-precision values will be used.

Bugfixes
############


:ref:`Release 6.11.0 <v6.11.0>`