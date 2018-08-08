.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Exports the results of an :ref:`EnggFitPeaks <algm-EnggFitPeaks-v1>`
fit to an HDF5 file indexed by bank ID. If multiple sets of fit
results are provided, then the file is divided into sub-groups for
each run number, with the **Run Number** groups indexed further by
bank ID. The results go in a sub-group of the **Bank** group called
**Single Peak Fitting**. This group contains one dataset, which is a
table of the fit parameters for every peak

Obtaining fit results for banks 1 and 2 of run 123456 and then saving
them with this algorithm would yield the following file structure:

.. diagram:: EnggSaveSinglePeakFitResultsToHDF5SingleRunHierarchy.dot


Obtaining fit results for banks 1 and 2 of run 123456 and bank 1 of
123457, and then saving them with this algorithm, would yield the
following file structure:

.. diagram:: EnggSaveSinglePeakFitResultsToHDF5MultiRunHierarchy.dot

Fit Parameters
##############

The fit function used in EnggFitPeaks is :ref:`Back2BackExponential
<func-BackToBackExponential>`, so the input table workspace must
contain values for the following parameters: ``["dSpacing", "A0",
"A0_Err", "A1", "A1_Err", "X0", "X0_Err", "A", "A_Err", "B", "B_Err",
"S", "S_Err", "I", "I_Err", "Chi"]``. Conveniently, this is exactly
what you get out of EnggFitPeaks.

Usage
-----

Ordinarily, we'd get our peak parameters table from EnggFitPeaks, but
we just mock one up here. See :ref:`EnggFitPeaks documenation
<algm-EnggFitPeaks-v1>` for how to generate this table.

**Example - Export fit params to a new HDF5 file:**

.. testcode:: EnggSaveSinglePeakFitResultsToHDF5

   import h5py
   import os

   peaks = CreateEmptyTableWorkspace()

   fit_param_headers = ["dSpacing", "A0", "A0_Err", "A1", "A1_Err", "X0", "X0_Err",
          	        "A", "A_Err", "B", "B_Err", "S", "S_Err", "I", "I_Err", "Chi"]

   [peaks.addColumn("double", header) for header in fit_param_headers]

   for i in range(3):
       peaks.addRow([(i + 1.0) / (j + 1) for j in range(len(fit_param_headers))])
 
   output_filename = os.path.join(config["defaultsave.directory"],
                                  "EnggSaveSinglePeakFitResultsToHDF5DocTest.hdf5")

   EnggSaveSinglePeakFitResultsToHDF5(InputWorkspaces=[peaks],
                                      Filename=output_filename,
                                      BankIDs=[1])

   with h5py.File(output_filename, "r") as f:
       bank_group = f["Bank 1"]
       peaks_dataset = bank_group["Single Peak Fitting"]
       print("Peaks dataset has {} rows, for our 3 peaks".format(len(peaks_dataset)))
       print("First peak is at D spacing {}".format(peaks_dataset[0]["dSpacing"]))
       print("Third peak X0 = {}".format(peaks_dataset[2]["X0"]))
   
.. testcleanup:: EnggSaveSinglePeakFitResultsToHDF5

   os.remove(output_filename)

Output:

.. testoutput:: EnggSaveSinglePeakFitResultsToHDF5

   Peaks dataset has 3 rows, for our 3 peaks
   First peak is at D spacing 1.0
   Third peak X0 = 0.5

.. categories::

.. sourcelink::
