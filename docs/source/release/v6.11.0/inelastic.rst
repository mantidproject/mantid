=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- Indirect/Inelastic interfaces have new option `Load Workspace History` in the settings dialog.

  .. figure::  ../../images/6_11_release/load_history_option.png
     :width: 500px

- :ref:`Elwin Tab <elwin>` of  :ref:`Data Processor Interface <interface-inelastic-data-processor>` now allows to access the Slice Viewer or 3D Plot from the OutputPlot widget of the output workspaces containing more than 1 histogram.
- :ref:`Convolution <convfit>` tab of the :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>` now has option ``Tie Peak Centres`` to tie two Lorentzians in the interface.
- `Function (Q)` tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` now allows to load `A0` fit parameter data into the interface.
- I(Q, t) tab in :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>` has new option `Output Composite Members`.
- Renamed tabs of :ref:`QENS Fitting <interface-inelastic-qens-fitting>` from ``MSD Fit``, ``I(Q,t) Fit``, ``ConvFit`` and ``F(Q)`` to ``MSD``, ``I(Q,t)``, ``Convolution`` and ``Function(Q)`` respectively. Added tooltip.

  .. figure::  ../../images/6_11_release/renamed_tabs.png
     :width: 350px

- :ref:`Container Subtraction <container-subtraction>` tab of the Corrections interface has new option not to delete subtracted workspaces when adding new data.


Bugfixes
--------
- Algorithm :ref:`BayesQuasi <algm-BayesQuasi>` not longer throws an ``index out of range`` error when using a sample with a Numeric Axis.
- :ref:`Elwin Tab <elwin>` of :ref:`Data Processor Interface <interface-inelastic-data-processor>` not longer freezes when running the tab.
- :ref:`Convolution <convfit>` :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>` interface no longer crashes when attempting to Fix all ``IsoDiffRot`` parameters from the ``EditLocalParameter`` dialog.
- Fixed comparison operation in ``ISISIndirectInelastic.OSIRISIqtAndIqtFitMulti`` system test by turning off uncertainty check, using absolute comparison, and adjusting the tolerance.
  This change does not affect general users.
- When the ADS is cleared from workspaces that are used to run fits on an open :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>`, a warning message now pops up when clicking on the `Run` button.
- Fixed a bug in the Monte Carlo error calculation on the I(Q, t) tab of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` where the first bin had an error of zero.
- :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` now supports loading data unrestricted by suffix if the option is selected from ``Settings``.
- :ref:`Elwin interface <elwin>` now allows to see the Spectrum number 0 on the widget plot of the selected preview workspace.
- :ref:`Elwin interface <elwin>` now plots the correct spectrum for the selected index when changing the preview spectrum above the plot widget combo box.
- :ref:`Moments<inelastic-moments>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface now have responsive sliders to changes in `Emin` and `EMax` properties when changed from the property browser.
- :ref:`Inelastic Bayes fitting <interface-inelastic-bayes-fitting>` interface now correctly calculates EISF errors on the Quasi tab.
- Adding new data to the  :ref:`Elwin data table <elwin>` after clearing the Analysis Data Service no longer raises a ``No data found`` warning.
- Ploting  a preview of the selected workspace on the :ref:`Elwin tab <elwin>` no longer crashes Mantid after that workspace has been deleted from the ADS.
- Fix a cutoff issue with ``Symmetric Energy Range`` label in :ref:`Iqt<iqt>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface.
- The dialog window for adding data in the  :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` no longer freezes when adding data.
- Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface no longer crashes by attempting to load a WorkspaceGroup rather than the expected Workspace2D.
- Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface no longer crashes by clicking `Run` before data has finished loading.
- Available fit functions in the `Function (Q)` tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface are now updated according to the type of data (`EISF`, `A0` or `Width`) loaded on the table.


Algorithms
----------

New features
############
- Abins/Abins2D algorithms now support "high-precision" eigenvectors from GAUSSIAN.
  These are activated in GAUSSIAN with the ``freq(HPModes)`` parameter.
  In previous Mantid versions the Abins parser would fail to read the resulting files; now the high-precision values will be used.


:ref:`Release 6.11.0 <v6.11.0>`
