.. _sans_gui_testing:

SANS GUI Testing
================

.. contents::
   :local:

Data reduction
--------------

*Preparation*

-  Get the ISIS sample data from the website
-  Ensure that the ISIS sample data directory is in the search directories


**Time required 20 - 30 minutes**

--------------

#. Open ``Interfaces`` > ``SANS`` > ``SANS v2``
#. Choose ``Load User File``; from the ISIS sample data in the ``loqdemo`` folder, choose ``Maskfile.txt``
#. Choose ``Load Batch File``; from the ISIS sample data in the `loqdemo` folder, choose ``batch_mode_reduction.csv``
#. In the ``Settings`` tab:
    - Go to ``Mask``
    - Click ``Display Mask``
    - This should give an instrument view with a circle at the centre
#. In the ``Settings`` tab:
    - Go to ``Q, Wavelength, Detector Limits`` sub-tab
    - Change the ``Phi Limit`` from 0 to 45 and uncheck ``use mirror sector``
    - Go to ``Mask`` sub-tab
    - Click ``Display Mask``
    - This should give an instrument view where only angles 0-45 are unmasked
    - Change the settings back to -90 - 90 and reselect ``use mirror sector``
    - In ``General, Scale, Event Slice, Sample`` sub-tab, set the `Reduction Mode`` to ``All``
#. In the ``Runs`` tab
    - Click ``Process All``
    - After some seconds the rows should turn green
    - In the Main window there should be a series of new workspaces; 4 group workspaces and 4 2D workspaces
    - Change the first column of the first row to 74045; click process selected
    - The row should turn blue; hovering over the row should give an error message
    - Change the first column of the first row back to 74044
    - Change the ``Reduction`` button to 2D
    - Check the ``Plot Results`` box
    - Click ``Process All``
    - A plot window will open; initially empty, then with a line
    - Change ``Reduction`` back to 1D
    - Click ``Process All``
    - In the plot you should end up with multiple lines plotted
    - Check the ``Multi-period`` box
    - 6 additional columns should appear in the table
    - Check that the ``Insert``, ``Delete``, ``Copy``, ``Paste``, ``Cut`` and ``Erase`` icons work as expected
    - Create multiple rows in the table with different data e.g. one row with an output name, one row without
    - Click the ``Export Table`` button and save the table as a csv file
    - Click ``Load Batch file`` and select the newly saved table. This should load without issue and the table look the same as when you saved it
    - Re-load the original batch file
#. In the ``Settings`` tab
    - In the ``General, Scale, Event Slice, Sample`` sub-tab, set ``Reduction Mode`` to ``Merged``
    - Return to the ``Runs`` tab
    - Ensure ``Plot results`` is checked
    - Click ``Process All``
    - This should result in a plot with three lines
#. In the ``Beam centre`` tab
    - Both the ``Update main-detector`` and ``Update Hab`` checkboxes should be checked and enabled
    - Click Run
    - A plot should appear after some seconds, with 4 lines
    - The 4 lines should gradually get closer together
    - This will run for some time, probably minutes
#. In the ``Sum Runs`` tab
        - Enter ``74044, 74019`` in the top line
        - Click ``Add`` at the side
        - Enter ``LOQ74044-add`` as Output file
        - Click ``Select Save Directory`` and select a directory in your managed paths
        - Click ``Sum`` at the bottom
        - Go back to the ``Runs`` tab
        - Remove all rows
        - Reload the batch file as before
        - Change the first column of both rows to ``LOQ74044-add``
        - Click ``Process All``
        - This should now process as before
#. In the ``Diagnostic Page`` tab
    - For run choose ``Browse`` and load the ``LOQ74044.nxs`` file
    - Click each of the ``Integral`` buttons
    - They should produce plots
    - Check the ``Apply Mask`` boxes and click the buttons again
    - They should produce new, slightly different plots
#. In the ``Runs`` tab
    - Check that all table, process, and load buttons have clear tooltips by hovering over them
    - Check that ``Zero Error Free`, ``Use Optimizations``, and ``Plot Results`` have clear tooltips