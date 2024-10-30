.. _sans_gui_testing:

SANS GUI Testing
================

.. contents::
   :local:

Data reduction
--------------

**Time required: about 1 hour**

--------------

Set up
######

#. Get the Training data from the downloads page on the Mantid Project website.
#. Open ``Interfaces`` > ``SANS`` > ``ISIS SANS``.
#. Click ``Manage Directories``. This opens Mantid's main dialog for managing
   search paths. Ensure the Training data directory is in the search
   directories.
#. If you haven't set one up yet, add a folder to save test data into later.
#. Set the default save directory in ``Manage Directories``
#. Click OK on the manage-directories dialog and ensure the ``Save Path`` on
   the SANS GUI (below batch file) displays the correct save path.
#. Click ``Load User File``; from the Training data in the ``loqdemo`` folder,
   choose ``MaskFile.toml``.
#. Click ``Load Batch File``; from the Training data in the ``loqdemo`` folder,
   choose ``batch_mode_reduction.csv``.

Automatic Save Selection
########################

#. Select ``File`` or ``Both`` from the ``Save Options`` at the bottom right of the screen.
#. In the ``Reduction`` section to the left of the ``Save Options``, switch between the ``1D``` and ``2D``` radio buttons..

   * When 1D is selected, ``CanSAS (1D)`` and ``NxCanSAS (1D/2D)`` should be checked.
   * When 2D is selected, only ``NxCanSAS (1D/2D)`` should be checked.

#. Check ``RKH (1D/2D)``.
#. Change the selected ``Reduction`` radio button.
#. The options should revert to the defaults above (with ``RKH (1D/2D)`` unchecked).
#. Select ``Memory``. The ``CanSAS (1D)``, ``NxCanSAS (1D/2D)``, and ``RKH (1D/2D)`` checkboxes should be disabled.
#. Swap between ``Memory`` and ``File`` with a ``2D`` reduction mode. ``CanSAS (1D)`` should always stay disabled.
#. Set the ``Save Option``` back to ``Memory`` to continue with the rest of the tests.

Runs table editing
##################

In the ``Runs`` tab:

#. Check that the ``Insert``, ``Delete``, ``Copy``, ``Paste``, ``Cut`` and
   ``Erase`` icons work as expected on table rows.
#. Tick the ``Sample Geometry`` button - some extra columns should appear.
#. Create multiple rows in the table with different data e.g. one row with
   an output name, one row without.
#. Try changing a random setting in the settings tab and remember what you set
   it to.
#. Make some more edits to the table. The settings will be reverted to the defaults
   set in the User File.
#. Click the ``Export Table`` button and save the table as a csv file. Check
   the file in an editor or Excel and ensure it looks like a sensible
   representation of the table in the format ``key,value,key,value,...``. All
   columns except ``Options`` and ``Sample Shape`` should be included.
#. Try unticking ``Sample Geometry`` and ticking ``Multi-period`` and resave the CSV. The
   displayed columns change but the saved file should contain the same set of
   columns regardless of whether these are ticked.
#. Click ``Load Batch file`` and select the newly saved table. All columns
   that were saved should be loaded.
#. Try deleting and/or reordering some of the columns in the saved file and
   re-load it. All of the values in the file should be populated in the correct
   columns.
#. Re-load the original batch file.

User files
##########

#. Change some values on the ``Settings`` tab and make a note of what you changed.
#. Re-load the user file and check the values you changed - they should have
   reverted to their original values.
#. Change some values on the ``Beam Centre`` tab. Re-load the user file. The inputs in the ``Centre Position``
   section should revert to their original values. The inputs in the ``Options`` section (such as the radius limits)
   should not revert.
#. Ensure that you can load the old style ``MaskFile.txt`` user file from the sample data.

   - **Note:** In order to see this file, you may need to change the settings in the file browser window to look for
     ``.txt`` files instead of ``.TOML`` files.

#. In the table on the ``Runs`` tab, under the ``User File`` column, enter
   ``MaskFile.toml`` in one row and ``MaskFile.txt`` in the other row. Click
   ``Process All``. After some seconds, the rows should turn green to indicate
   that they processed successfully.
#. Re-load the original user and batch files as per the set-up instructions.

Display mask
############

In the ``Settings`` tab:

#. Go to ``Mask``.
#. Click ``Display Mask``.
#. This should give an instrument view with a circle at the centre.
#. Close the Instrument View window
#. Go to ``Q, Wavelength, Detector Limits`` sub-tab.
#. Change the ``Phi Limit`` to read 0 to 45 and uncheck ``use mirror sector``.
#. Go to ``Mask`` sub-tab.
#. Click ``Display Mask``.
#. This should give an instrument view where only angles 0-45 are unmasked.
#. Change the settings back to -90 to 90 and reselect ``use mirror sector``.

Processing
##########

*1D reduction*

#. Clear all workspaces in your workspaces list if they are not empty.
#. In ``General, Scale, Event Slice, Sample`` sub-tab, ensure the ``Reduction
   Mode`` is ``All``.
#. In the ``Runs`` tab, under ``Save Options``, select ``Both``, and tick
   ``CanSAS (1D)`` and ``NXcanSAS (1D/2D)``.
#. Select ``Save Can``.
#. Click ``Process All``.
#. After some seconds the rows should turn green.
#. In the workspaces list, there should be a series of new workspaces; four
   group workspaces and four 1D workspaces.
#. Check your default save directory. For each reduction two banks (HAB/main) should
   be saved. In total, there should be 20 workspaces saved. For each row, file type, and bank, there should be a
   reduced file (with no suffix) and a ``sample`` file. The ``first_time`` line should also produce a ``can`` workspace
   for each file type and bank. This is because both workspaces have the same ``can`` input run numbers and so the
   reduction only calculates it once.
#. Double-click the 1D workspaces and you should get a single line plot.
#. Clear the newly created files and workspaces to make the next test easier
#. Change the contents of the first cell in the first row to ``74045`` and click
   ``Process Selected``.
#. The row should turn blue; hovering over the row should give an error message.
#. Change the first column of the first row back to ``74044``.
#. Click on another row, the modified row should have cleared its colour

*2D reduction*

#. Clear all workspaces in your workspaces list if they are not empty.
#. In ``General, Scale, Event Slice, Sample`` sub-tab, ensure the ``Reduction
   Mode`` is ``All``.
#. Switch to the 2D ``Reduction Mode``.
#. Click ``Process All``.
#. You should get four 2D workspaces instead of the previous 1D workspaces
   (they will have 100 spectra instead of 1). Double-click them and check you
   can do a colourfill plot.
#. Check your save directory. There should now only be a ``.h5`` file for each
   output.
#. Clear the newly created files and workspaces to make future tests easier
#. Change ``Reduction`` back to 1D.
#. Click ``Process All``.
#. When it completes, Check the ``Multi-period`` box - six additional columns should appear in the table.
#. Delete all rows and re-load the batch file.

*Merged reduction*

#. In the ``Settings`` tab, ``General, Scale, Event Slice, Sample`` sub-tab,
   set ``Reduction Mode`` to ``Merged``.
#. Return to the ``Runs`` tab.
#. Ensure save outputs ``CanSAS (1D)`` and
   ``NXcanSAS (1D/2D)`` are ticked.
#. Click ``Process All``.
#. The workspaces list should now contain a group named
   ``LAB_and_HAB_workspaces_from_merged_reduction`` that contains the ``main``
   and ``HAB`` workspaces, which were previously ungrouped for a non-merged
   reduction.
#. Check your save directory. As well as the previous 1D outputs, there should
   now be an additional ``.xml`` and ``.h5`` output file for the merged output
   for each row.
#. In the ``Settings`` tab, ``General, Scale, Event Slice, Sample`` sub-tab,
   change the ``Reduction Mode`` back to ``All``.

*Scaled Background Subtracted Reduction*

#. Create a new copy of the User File in your file browser.
#. Using a text editor. Open this new copy in a text editor and find the ``[detector.configuration]`` section.
#. Under this section, make sure setting selected_detector is set to ``Merged``.
#. Back in the ISIS SANS interface, change the user file to this new file.
#. Click over to the ``Runs`` tab.
#. Set the ``Save Options`` to ``Memory``.
#. Select one of the rows and click ``Process Selected``
#. Take note of the name of the reduced workspace with ``merged`` in the title.
#. Make a copy of the row you just processed using the ``Copy`` and ``Paste`` buttons above the runs table.
#. Change the ``Output Name`` of the new row to something like ``bgsub_test``.
#. Check the ``Scaled Background Subtraction`` checkbox.
#. In the ``BackgroundWorkspace`` column, enter the name of the merged workspace you took note of before.
#. In the ``ScaleFactor`` column, enter ``0.9``.
#. Set the ``Save Options``` to Both and ensure that save outputs ``CanSAS (1D)`` and ``NXcanSAS (1D/2D)`` are ticked
#. Select this new row and click ``Process Selected``.
#. When it completes, two output files should have been created with ``bgsub_test`` in the name. One, which is the
   normal output data. Another with the scaled subtraction, which should have ``_bgsub`` appended to the name.
#. Right click on each of these and select ``Show Data``. The subtracted workspace's values should be 10% of the of the
   unsubtracted workspace's values.
#. Check that your save location contains files for both the background subtracted workspace and the normal reduction
   output.

Save Other
##########

*Single Workspace*

#. Navigate to the ``Runs`` tab, making sure there are some reduced workspaces present in the ADS. Follow one of the
   "Processing" instruction sets above if you need to create some.
#. Click the ``Save Other`` button.
#. Select one of the workspaces from the list.
#. Provide a path to a new save directory, and provide a file name.
#. Click ``Save``.
#. Check the file was saved to the correct location on your system.

*Multiple Workspaces*

#. Select multiple workspaces with Shift or Ctrl/Cmd.
#. Provide a suffix for the files.
#. Click ``Save``.
#. Check that the files were saved with their workspace's names, but with the provided suffix appended.

Beam centre finder
##################

In the ``Beam centre`` tab:

#. Make a note of the four values representing the rear/front detector centre positions.
#. Check that the ``Find Rear Centre`` radio button is selected and click run.
#. A plot should appear - make sure to show it if it is behind another window. It should be updated with four lines, which gradually get closer together. This might take a while to run.
#. Check the values in the first two text boxes at the top (Centre Position - Rear) have changed when it has finished running.
#. Select the ``Find Front Centre`` radio button and re-run the test.
#. Four more lines should appear on the same plot. This time, in the values at the top, only the values for the front should have changed.

Sum runs
########

In the ``Sum Runs`` tab:

#. Enter ``74044, 74019`` in the top line.
#. Click ``Add`` at the side.
#. Check that ``LOQ74044-add`` is automatically entered as the Save File at the bottom of the tab.
#. At the top-right of the tab, click the ``Select Save Directory`` button and select a directory in your managed paths.
#. Click ``Sum`` at the bottom.
#. Go back to the ``Runs`` tab.
#. Remove all rows.
#. Reload the batch file as before.
#. Change the first column of both rows to ``LOQ74044-add``.
#. Click ``Process All``.
#. This should now process as before.

Diagnostics
###########

In the ``Diagnostic Page`` tab:

#. For run choose ``Browse`` and load the ``LOQ74044.nxs`` file.
#. Click each of the ``Integral`` buttons.
#. They should produce plots.
#. Check the ``Apply Mask`` boxes and click the buttons again.
#. They should produce new, slightly different plots.

Display
#######

#. In the ``Runs`` tab, check that all table, process, and load buttons have
   clear tooltips by hovering over them.
#. Check that ``Zero Error Free`` and ``Use Optimizations`` have clear tooltips.
#. In the settings, hover over a random selection of buttons and text boxes to check tooltips are still there.
   Users rely on the tooltips a lot and really do notice each missing one.
   *Note: The* ``Wavelength`` *section of the settings is missing its tooltips. We and the users are aware of this so an
   issue should not be made when it is discovered.*
