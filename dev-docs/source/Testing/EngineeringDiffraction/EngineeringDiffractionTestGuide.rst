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
This test follows the simple steps for calibrating and focusing in the Engineering Diffraction 2 Gui.

Calibration
-----------

1. Ensure you are able to access the archive.

2. Open the Engineering Diffraction 2 gui.

3. On opening the gui the Create New Calibration option should be selected.

4. Open the setting dialog from the cog in the bottom left of the gui.

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

5. Check that in your save location there is a Calibration folder containing six files
   `ENGINX_305761_bank_1` and `ENGINX_305761_bank_2` for each of `.dat`, `.gss`, and `.nxs`.

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

2. In the Engineering Diffraction 2 settings tick the Force Vanadium Recalculation.

3. Calibrate again. It should take a longer time to perform as it does the entire calibration again.

Test 4
^^^^^^

This test covers the Cropping ability

1. Change the RB Number to North.

2. Tick the Crop Calibration option. In the select Bank/Spectra select `1 (North)`

3. Click calibrate.

4. Go to focus tab and do the same with the Crop Focus.

5. Change the RB number to custom.

6. Repeat steps 2-4 this time using Custom Spectra `1200-1400`