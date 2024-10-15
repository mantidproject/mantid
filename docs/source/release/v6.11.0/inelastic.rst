=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- Indirect/Inelastic interfaces have a new ``Load Workspace History`` option in the settings dialog.

  .. figure::  ../../images/6_11_release/load_history_option.png
     :width: 500px

- The :ref:`Elwin Tab <elwin>` of  :ref:`Data Processor Interface <interface-inelastic-data-processor>` now includes access the Slice Viewer or 3D Plot from the ``OutputPlot`` widget of the output workspaces containing more than 1 histogram.
- The :ref:`Convolution <convfit>` tab of the :ref:`Inelastic QENS Fitting Interface <interface-inelastic-qens-fitting>` now has option ``Tie Peak Centres`` to tie two Lorentzians in the interface.
- The ``Function (Q)`` tab of the :ref:`QENS Fitting Interface <interface-inelastic-qens-fitting>` now allows to load `A0` fit parameter data into the interface.
- The ``I(Q, t)`` tab in :ref:`Inelastic QENS Fitting Interface <interface-inelastic-qens-fitting>` has new option `Output Composite Members`.
- Renamed tabs of :ref:`QENS Fitting <interface-inelastic-qens-fitting>` from ``MSD Fit``, ``I(Q,t) Fit``, ``ConvFit`` and ``F(Q)`` to ``MSD``, ``I(Q,t)``, ``Convolution`` and ``Function(Q)`` respectively. Added tooltip.

  .. figure::  ../../images/6_11_release/renamed_tabs.png
     :width: 350px

- The :ref:`Container Subtraction <container-subtraction>` tab of the Corrections interface has new option not to delete subtracted workspaces when adding new data.


Bugfixes
--------
- Algorithm :ref:`BayesQuasi <algm-BayesQuasi>` no longer throws an ``index out of range`` error when using a sample with a numeric axis.
- The :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` no longer freezes when running the tab.
- The :ref:`Convolution <convfit>` of the :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>` no longer crashes when attempting to fix all ``IsoDiffRot`` parameters from the ``EditLocalParameter`` dialog.
- When the ADS is cleared of workspaces that are used to run fits on an open :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>`, a warning message now pops up when clicking on the `Run` button.
- Fixed a bug in the Monte Carlo error calculation on the I(Q, t) tab of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` where the first bin had an error of zero.
- The :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` now supports loading data unrestricted by suffix if the option is selected from ``Settings``.
- The :ref:`Elwin interface <elwin>` now allows to see the Spectrum number 0 on the widget plot of the selected preview workspace.
- The :ref:`Elwin interface <elwin>` now plots the correct spectrum for the selected index when changing the preview spectrum above the plot widget combo box.
- The :ref:`Moments<inelastic-moments>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface now have responsive sliders to changes in ``Emin`` and ``EMax`` properties when changed from the property browser.
- The :ref:`Inelastic Bayes fitting <interface-inelastic-bayes-fitting>` interface now correctly calculates EISF errors on the Quasi tab.
- Adding new data to the  :ref:`Elwin data table <elwin>` after clearing the Analysis Data Service no longer raises a ``No data found`` warning.
- Plotting a preview of the selected workspace on the :ref:`Elwin tab <elwin>` no longer crashes Mantid after that workspace has been deleted from the ADS.
- Fix a cutoff issue with ``Symmetric Energy Range`` label in the :ref:`Iqt<iqt>` tab of the :ref:`Data Processor <interface-inelastic-data-processor>` interface.
- The dialog window for adding data in the :ref:`Elwin Tab <elwin>` of the :ref:`Data Processor Interface <interface-inelastic-data-processor>` no longer freezes when adding data.
- Fixed a crash on the Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface caused by attempting to load a WorkspaceGroup rather than the expected Workspace2D.
- Prevented a crash on the Quasi tab of the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface caused by clicking ``Run`` before data has finished loading.
- Available fit functions in the ``Function (Q)`` tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface are now updated according to the type of data (``EISF``, ``A0`` or ``Width``) loaded in the table.


Algorithms
----------

New features
############
- :ref:`Abins <algm-Abins>`/:ref:`Abins2D <algm-Abins2D>` algorithms now support "high-precision" eigenvectors from GAUSSIAN.
  These are activated in GAUSSIAN with the ``freq(HPModes)`` parameter.
  In previous Mantid versions the Abins parser would fail to read the resulting files; now the high-precision values will be used.


:ref:`Release 6.11.0 <v6.11.0>`
