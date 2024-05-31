=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- Added new `TeixeiraWaterIqt` fitting function, to fit linewidth and molecular residence time in intermediate scattering functions with Teixeira's model for water. This is now available in the IqtFit tab of the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface.
- Added new workspace dialog widget with the capacity to select and open multiple compatible workspaces at once. For use in :ref:`Elwin Tab <elwin>` of  :ref:`Data Manipulation Interface <interface-inelastic-data-manipulation>`
- Fix a crash happening when setting spectra limits on workspace dialogs using an string format of the sort: "SpectraStart-SpectraEnd" where Spectra Start is larger than Spectra End.
- Added the possibility to make an external call from selected Inelastic/Indirect interfaces for plotting data as a 3D Surface.
- The IqtFit tab of :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>` now has a 'Fit Type' box for selecting a Stretched Exponential function. This allows further fit functions to be added in the future.
- On :ref:`Elwin Tab <elwin>` of  :ref:`Data Manipulation Interface <interface-inelastic-data-manipulation>`, it is now possible to display in the workspace table either one row per spectra, or just one row per workspace.
- Add documentation techniques page on :ref: `Applying absorption corrections` <applying_corrections>.
- The files tab of the :ref:`Elwin Tab <elwin>` of  :ref:`Data Manipulation Interface <interface-inelastic-data-manipulation>` has been removed. Workspaces can still be added from files in the multifile dialog of the workspaces tab.
- It is now possible to use the :ref:`Inelastic Bayes Fitting <interface-inelastic-bayes-fitting>` interface on a MacOS operating system.
- The `Inelastic Data Analysis` interface has been renamed to the :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>` interface. The new name provides a better description of what it does.
- The files tab of the :ref:`Elwin Tab <elwin>` of  :ref:`Data Manipulation Interface <interface-inelastic-data-manipulation>` has been removed. Workspaces can still be added from files in the multifile dialog of the workspaces tab.
- A checkbox option is added in Data Manipulation with title "EnfoceNormalization". This option is set to True by default (no change to the current algorithms/workflow occurs). When it is set to False, the LHSWorkspace from the output from ExtractFFTSpectrum is used in both branches to perform the final workspace division and the two intermediate workspace divisions are skipped.
- A deprecation warning has been added to the Calculate Paalman Pings tab of :ref:`Inelastic Corrections interface <interface-inelastic-corrections>`. This tab will be removed in two minor releases time if we are not informed otherwise.
- The :ref:`ElasticWindowMultiple <algm-ElasticWindowMultiple>` algorithm will add the integration range to the output workspaces sample logs, calling it either from script or from the :ref:`Elwin Tab <elwin>` of :ref:`Data Manipulation Interface <interface-inelastic-data-manipulation>`.


Bugfixes
--------
- Fix a crash in the :ref:`Elwin Tab <elwin>` of the Data Manipulation Interface ocurring when all items of the table were selected with the keyboard and the clicked on `Remove Selected` button.
- Add `Select All` push button on :ref:`Elwin Tab <elwin>` to select all rows when clicked.
- Fixed a bug where it wasn't possible to use a custom fit function on the IqtFit tab of :ref:`Inelastic QENS Fitting <interface-inelastic-qens-fitting>`.
- Disable `Add` button and change the button text to *Loading* from workspace dialogs to prevent warnings or crashes if `Add` button is pressed but files are still loading.
- Fixed a bug that crashed Mantid when calling Open Slice Viewer from Indirect/Inelastic interfaces when there are less than two histograms in the workspace.
- Fixed a bug crashing Mantid on the Fit Property Browser of :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface when trying to set a fit function with a parameter having a tie to itself.
- It is now approximately 30% faster to load data into the F(Q)Fit tab in the :ref:`QENS Fitting interface <interface-inelastic-qens-fitting>`.
- Fixed a bug causing the Full Function Browser and Template Function Browser from going out of sync on the :ref:`QENS Fitting <interface-inelastic-qens-fitting>` interface.
- Fix bug in the :ref:`Elwin Tab <elwin>` where the "Background Subtraction" and "Normalise to Lowest Temp" properties were ignored


Algorithms
----------

New features
############
- A checkbox option is added in TransformToIqt/CalculateIqt with title "EnfoceNormalization". This option is set to True by default (no change to the current algorithms/workflow occurs). When it is set to False, the LHSWorkspace from the output from ExtractFFTSpectrum is used in both branches to perform the final workspace division and the two intermediate workspace divisions are skipped.

Bugfixes
############


:ref:`Release 6.10.0 <v6.10.0>`