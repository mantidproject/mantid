.. _Engineering_Diffraction_TestGuide-ref:

Engineering Diffraction Testing
=================================

.. contents:: Table of Contents
    :local:

Preamble
^^^^^^^^^
This document is tailored towards developers intending to test the Engineering Diffraction
interface.

Runs can be loaded from the archive, however it is possible that different run numbers
will be needed as older runs may be deleted.

Overview
^^^^^^^^
The Engineering Diffraction interface allows scientists using the EnginX instrument to interactively
process their data. There are 3 tabs which are ordered according to the main steps performed.
These are:

- Calibration - This is where a vanadium and cerium oxide run are entered to calibrate the subsequent data.
- Focus - Where are the data across multiple spectra are summed into a single spectrum for later steps.
- Fitting - Currently a work in progress, no testing is required

Test 1
^^^^^^
This test follows the simple steps for calibrating and focusing in the Engineering Diffraction Gui.

Calibration
-----------

1. Ensure you are able to access the archive.

2. Open the Engineering Diffraction gui.

3. On opening the gui the Create New Calibration option should be selected.

4. Open the settings dialog from the cog in the bottom left of the gui.

5. Set the Save location to a directory of your choice.

6. For the vanadium number enter `307521`, and for the Calibration Sample number enter `305738`.

7. Tick the Plot Calibrated Workspace option.

8. Click Calibrate, after completing calibration it should produce two plots.

.. image:: /images/EngineeringDiffractionTest/EnggDiffExpectedVanCurve.png
    :width: 900px

.. image:: /images/EngineeringDiffractionTest/EnggDiffExpectedLinear.png
    :width: 900px

9. Check that in your save location there is a Calibration folder containing three files
   `ENGINX_307521_305738` with the suffixes `_all_bank`, `_bank_North`, `_bank_South`, and
   a Vanadium_Runs folder containing two files: `307521_precalculated_vanadium_run_bank_curves`
   and `307521_precalculated_vanadium_run_integration`.

Focus
-----

1. Change to the Focus tab.

2. For the Sample Run number use `305761`.

3. Tick the Plot Focused Workspace option.

4. Click Focus, after completing calibration it should produce a plot.

.. image:: /images/EngineeringDiffractionTest/EnggDiffExampleFocusOutput.png
    :width: 900px

5. Check that in your save location there is a Focus folder containing six files
   `ENGINX_305761_bank_1` and `ENGINX_305761_bank_2` for each of `.abc`, `.gss`, and `.nxs`.

Test 2
^^^^^^

This test covers the RB number.

1. Enter a string into the RB number box.

2. Follow the steps of Test 1, any output files should now be located in [Save location]/user/[RB number]

Test 3
^^^^^^

This test covers the Force Vanadium Recalculation functionality.

1. With the previous setup run calibration again. It should happen much faster as it loads
   the previous calibration.

2. In the Engineering Diffraction settings tick the Force Vanadium Recalculation.

3. Calibrate again. It should take a longer time to perform as it does the entire calibration again.

4. Check that the "Last Modified" timestamp on the files in the `Vanadium_Runs` folder and ensure that they have
   been updated.

Test 4
^^^^^^

This test covers the Cropping functionality.

1. Change the RB Number to "North", this is purely to separate the cropped output files into their own space.

2. Tick the Crop Calibration option. In the select Bank/Spectra select `1 (North)`

3. Check the "Plot Calibrated Workspace" checkbox and click calibrate.

4. The two generated figures should be one with two vanadium plots and one with a single TOF Peaks plot.

5. Check that only a single output file was generated.

6. Go to focus tab and do the same with the Crop Focus. In comparison to the previous focus test, this will only
   generate a single axis on the figure, rather than two.

7. Change the RB number to "Custom".

8. Repeat steps 2-5 this time using Custom Spectra `1200-1400`. Please note that some custom spectra values may
   cause the algorithms to fail.


Test 5
^^^^^^

This test covers the loading and plotting focussed data in the fitting tab. It is advisable to have at least two focussed datasets for the subsequent tests: this could be the two banks of run 305761 already generated, but a better test would be to use focussed data for runs 305793-305795 which have different stress and strain log values.

1. Navigate to one or more focussed .nxs files in the `Focussed Run Files` box

2. Click the `Load` button. A row should be added to the UI table for each focussed run.There should be a grouped workspace with the suffix `_logs` in the ADS with tables    corresponding to each log value specified in the settings (to open the settings use the cog in the bottom left corner of the UI). Each row in these tables should correspond to the equivalent row in the UI table. There should be an additional table called `run_info` that provides some of the metadata for each run.

3. The log values that are averaged can be selected in the settings (cog button in the bottom left corner of the UI). Change them and close the UI. Open a new instance of the UI to check these settings have been remembered. note that any change to the selected logs won't take effect in the current session.

4. Repeat steps 1-2 above but this time try checking the `Add To Plot` checkbox, when loading the run(s) the data should now be plotted and the checkbox in the `Plot` column of the UI table should be checked.

5. Repeat steps 1-2 again but try changing the x-unit

6. Plot some data and un-dock the plot in the UI by dragging or double-clicking the bar at the top of the plot labelled `Fit Plot`. The plot can now be re-sized.

7. To dock it double click the `Fit Plot` bar (or drag to the bottom of the toolbar). You may want to un-dock it again for subsequent tests.

Test 6
^^^^^^

This tests the removal of focussed runs from the fitting tab.

1. Having loaded multiple runs, select a row in the UI table and then click the `Remove Selected` button below the table. The row should be removed, if the run was plotted it will disappear from the plot and there should be one less row in each of the log tables with each row corresponding to the run in the same row of the UI table. The workspace of the focussed run that was removed from the UI will still exist in the ADS.

2. Try clicking the `Remove All` button, the UI table should be empty and the log workspaces no longer present.

3. Try loading in a run again, the UI should still be able to access the workspace and remember the log values - check there are no calls to ``AverageLogData`` in the log (should be visible at notice level).

4. Try removing a workspace by deleting it in the ADS, the corresponding row in the log tables and the UI table should have been removed.

Test 7
^^^^^^

This tests that the background subtraction works.

1. load in a run and check the `Subtract BG` button in the UI table. This should generate a workspace with suffix `_bg` and the data should look like the background is flat and roughly zero on the plot using the default parameters (other columns in the UI table).

2. Select the row in the table for which the background has been subtracted, the `Inspect Background` button should now be enabled, click it to open a new figure which shows the raw data, the background and the subtracted data. Changing the values of Niter, BG, XWindow and SG (input to ``EnggEstimateFocussedBackground``, hover over a cell inn the table to see a tool tip for explanation) should produce a change in the background on the external plot and in the UI plot.

Test 8
^^^^^^

This tests the operation of the fit browser.

1. Check that when no data are plotted the `Fit` button on the toolbar does nothing.

2. Plot more than one focussed run with xunit of TOF and click the `Fit` button. A simplified version of the standard mantid fit browser should now be visible.

3. You should be able to select runs in the Settings > Workspace combo box. If you remove a run the combobox should update. Try adding some peaks (for testing purposes add different types) and a background by right-clicking on the plot. If BackToBackExponential peaks are used then the A,B parameters should be fixed automatically for ENGIN-X data.

4. Perform a fit by clicking Fit>Fit in the fit browser. On completion of the fit, a group workspace with suffix `_fits` should have appeared in the ADS. In this group of workspaces there should be a matrix workspace for each parameter fitted (named by convention FunctionName_ParameterName), to view this right-click on the ADS and `Show Data`. For any runs not fit there should be a NaN value in the Y and E fields. In addition there is a workspace that has converted any peak centres from TOF in d-spacing (suffix `_d`). There should be an additional table called `model` that summarises the chisq value and the function string including the best-fit parameters.

5. The function string including the best-fit parameters should also have been automatically saved as a custom setup in the fit browser (Setup > Custom Setup). To inspect the fit for a given run, select a custom setup and the values in the fit property browser should update, now click Fit > Evaluate Function.

Test 9
^^^^^^

This tests the sequential fitting capability of the UI (where the result of a fit to one workspace is used as the initial guess for the next).

1. Load in several focussed runs (preferably some that differ by a log value, e.g. 305793-305795).

2. Plot a single run, open the fit browser and input a valid fit function (either manually or from Setup > Custom Setup).

3. The `Sequential Fit` button should now be enabled. Click it and the  group of fit workspaces should appear in the ADS, each with a row for each of the runs in the table. All the runs should have been fitted.

4. The order of the runs in the sequential fit should be obtainable from the log at notice level - check that this corresponds to the order of the average value of the primary log (which can be selected in the settings, cog in the bottom left corner of the UI).

5. Try changing the primary log and the order (ascending/descending) in the settings (note that leaving the primary log blank will make the Sequential fit use the order of the runs in the UI table). Repeat the steps above to check that the sequential fit is operating in the expected order.

6. Close the UI and open a new instance, it should remember the primary log and the order.
