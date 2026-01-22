# Reflectometry GUI Testing

```{contents}
:local:
```

## Introduction

The ISIS Reflectometry interface is a graphical front end for the main
ISIS reflectometry reduction algorithm
`ReflectometryISISLoadAndProcess <algm-ReflectometryISISLoadAndProcess>`
and post-processing algorithm `Stitch1DMany <algm-Stitch1DMany>`. The
purpose of the interface is to speed up the analysis of the
Reflectometry spectra by providing batch-mode execution of these
algorithms on groups of runs.

## Set up

1.  These instructions assume use of MantidWorkbench.
2.  Ensure you have the [ISIS Sample
    Data](http://download.mantidproject.org) in your Mantid user
    directories.
3.  Ensure you have the ISIS archive enabled.
4.  Open the ISIS Reflectometry interface.
5.  Go to the menu `Tools->Options` and ensure all of the warnings are
    ticked. On the Rounding tab, set the precision to 2 decimal places.

## Loading batch files

1.  Go to the menu `Batch->Load`.
2.  Browse to the sample data and select file `INTER_NR_test.json`.
3.  The interface should be populated with some information, including
    some content in the runs table.
4.  Click the `Expand All Groups` button on the toolbar to expand the
    groups in the table.
5.  The decimal values should be shown to 2 decimal places as per the
    Options that we set.
6.  Re-open the same batch file. You should not be prompted with any
    warnings (unless you changed anything).

## Adding and deleting batch tabs

1.  The interface should open with two batch tabs created by default.
2.  Click the cross on any tab to delete it.
3.  Check that if all batch tabs are deleted from the interface then a
    new one is automatically added.
4.  Check that you can create a new batch tab using the menu
    `Batch->New` option.

## Changing the Default instrument

1.  At the top-left and bottom-right of the interface is a drop-down
    menu called `Instrument` - it can switch between INTER, SURF, CRISP,
    POLREF, and OFFSPEC. Try changing it.
2.  This should change the instrument for the whole of Mantid - check it
    by going to `File->Settings` in the MantidWorkbench main menu.
3.  Reload the batch file. This should change the instrument back to
    `INTER`.

## Processing Rows

1.  Re-load the batch file to reset the state.
2.  Select a single row within a group with the mouse.
3.  Click Process. Some output workspaces should be produced and the row
    should turn green. (You need to click off the row to see the green
    highlighting.)
4.  The process bar along the bottom should go from empty to full.
5.  Repeat this test for multiple rows (selecting using Shift and Ctrl
    while clicking with the mouse - a bit like in Excel)
6.  Once you have processed all rows inside a group, the group header
    will turn pale green (paler than the rows to indicate the contents
    are complete but post-processing has not been done).

## Processing Groups

1.  Re-load the batch file to clear the state and expand the groups.
2.  Click on a group header line and clicking Process. This will process
    all rows in the group as well as post-processing the group itself.
3.  The group header will turn green (the same shade as the rows).
4.  Now click off the table to deselect everything, then click Process.
5.  You will get a warning about processing the whole table - click ok.
6.  This will now process anything that was left unprocessed.

## Generating Plots

There are two plotting buttons on the toolbar - they look like little
plots. The first one plots the individual rows (the reduced output) and
the second one plots the post-processed (stitched) output for groups.

Assuming the whole table is processed:

1.  Select a group header:
    - Plot the reduced data (first plot button) - this plots two lines
      for the individual reduced rows.
    - Plot the stitched data (second plot button) - this plots a single
      line for the stitched output.
2.  Select a processed row:
    - Plot the reduced data - this plots one line for that reduced row.
    - Plot the stitched data - this does nothing for a row.
3.  We can also over-plot outputs from different groups. Select the
    whole table with `Ctrl-A`:
    - Plot the reduced data - this plots 4 lines (reduced output for all
      rows).
    - Plot the stitched data - this plots 2 lines (stitched output for
      both groups).

## Polarisation Corrections

Polarisation corrections settings are under the `Experiment Settings`
tab in the interface. These should only work with `CRISP`, `POLREF` or
`OFFSPEC`.

1.  In the Runs tab set the instrument to `INTER`.
2.  On the `Experiment Settings` tab, the `Polarisation Corrections`
    combo box should be greyed out.
3.  Go back to the Runs tab and set the instrument to `OFFSPEC`.
4.  Back on the `Experiment Settings` tab, the
    `Polarisation Corrections` combo box should now be enabled and the
    `Polarization Efficiencies` combo box should be disabled.
5.  Select `Parameter File` from the `Polarisation Corrections` combo
    box. The `Polarization Efficiencies` combo box should still be
    disabled.
6.  Switch to `Workspace` from the `Polarisation Corrections` combo box.
    The `Fredrikze Input Spin State Order` and
    `Polarization Efficiencies` combo boxes should become enabled. The
    latter should show a list of all loaded workspaces.
7.  Switch to `FilePath` from the `Polarisation Corrections` combo box.
    `Polarization Efficiencies` should now appear as a line edit. It
    should appear red for invalid paths and white for valid paths on
    your system.
8.  Switch back to the `ParameterFile` setting from the
    `Polarisation Corrections` combo box.
9.  Back on the `Runs` tab, delete all rows in the table (this can be
    done by pressing `Ctrl-A` and then `Delete`).
10. Note that this will leave an empty row. In that row enter run number
    `44956` and angle `0.4`.
11. Check you can process the row and it turns green.

## Search by experiment

1.  Clear all rows in the Runs table and set the instrument to INTER.
2.  In the Search box on the left, enter Investigation Id `1120015` and
    Cycle `11_3`.
3.  Click `Search` and the results list will fill with all the runs for
    that experiment.
4.  Try selecting some of the results and clicking the `Transfer`
    button.
5.  The run numbers should be filled into the main Runs table following
    these rules:
    - Runs with the **same title** will appear in the **same group**.
    - Runs in the same group with **different angles** will appear on
      **separate rows**.
    - Runs in the same group with the **same angle** will be combined
      into the **same row**, and shown as a sum e.g. `13460+13462`.
    - Runs that are highlighted blue are invalid and will not be
      transferred. Hover over them to see a message explaining why.
    - If the runs are already in the table, they will not be transferred
      again (i.e. no duplicates).

## Copy, Cut and Paste

Copy, Cut and Paste are available from the toolbar buttons, the
right-click content menu, or the standard Ctrl- C, X, and V shortcuts.

Note that it is very picky about pasting onto the correct destination
(i.e. group onto group, row onto row etc.) and gives a confusing error
message about "depth and size" if you get it wrong - this just indicates
that the operation is not possible.

Re-load the test batch file and then test the operations listed below.

These should work:

- Select a row, copy it, and paste it onto a different row.
- Select 2 rows from one group, copy them, and paste onto 2 rows in
  another group.
- Select a group and copy it. Paste it onto another group.
- Copy a group. Deselect everything in the table before you paste. It
  should be pasted as a new group at the end.

These will give an error:

- Copy a row. Deselect everything and paste. This fails because we don't
  know which group to paste into.
- Copy a row and try pasting onto a group. This could in theory append
  the row into the group but is currently not implemented.
- Copy a group and try pasting onto a row. This is not possible.

## Discarding changes

1.  Re-load the test batch file.
2.  Edit the data in the table e.g. change an angle.
3.  Close the interface using the <span class="title-ref">x</span>
    button at the top.
4.  You should be warned that unsaved changes will be lost. Click Cancel
    and nothing should be lost.
5.  Try again and click OK. The interface should close and discard your
    changes.
6.  Repeat the test but instead of closing the interface, now attempt to
    re-load the batch file.
7.  You should be warned that this will discard your unsaved changes.
    Again, the OK and Cancel options should work as expected.

## Saving a batch

1.  Load the test batch file.
2.  Make some changes to the table or settings that you can easily
    remember.
3.  Go `Batch->Save`. A Save dialog will appear. Select a file to save
    to and OK it.
4.  Close and re-open the interface. Note that you should *not* be
    prompted about discarding unsaved changes.
5.  Load your saved batch file and check that the items you changed are
    restored.

## Save tab

1.  Close the Reflectometry GUI and re-open it to ensure any previous
    settings have been cleared.
2.  In the Search box on the left, enter Investigation Id `1120015` and
    Cycle `11_3`.
3.  Click `Search` and the results list will fill with all the runs for
    that experiment.
4.  Select one of the results that is not highlighted blue (i.e. run
    `11934`) and click the `Transfer` button.
5.  In the main Runs table, click to process the row.
6.  Go to the Save tab and hit Refresh. The workspaces list will contain
    all of the workspaces in the ADS.
7.  Select a workspace in the list that starts with `IvsQ`.
8.  Type a valid path into the Save path textbox.
9.  Type something in the prefix field you'd like to use to identify the
    file. *The files are saved in the form
    \[prefix\]\[workspace\]\[ext\]*.
10. In the File Format section, select `Custom format (*.dat)`. Check
    that option `Additional columns (includes Q resolution)` is ticked
    but disabled, as it is not applicable.
11. Untick `Header` and `Q resolution` and set the separator to `Comma`.
12. Click `Save` and open the file that should have been saved to the
    save directory you specified. It should contain 3 columns of
    numbers, separated by commas.
13. Tick `Q resolution` and re-save. It should now contain 4 columns of
    numbers.
14. Double-click on a workspace name in the left list, e.g.
    `IvsQ_binned_11934`. The right list should be populated with
    parameters but be disabled.
15. Tick `Header` and the parameters list should be enabled. Select a
    couple of them, e.g. `nperiods` and `run_start`, and re-save.
    - The file should now contain some header text starting with `MFT`.
    - Amongst other things this text should contain the logs you
      selected, e.g. `nperiods : 1` and `run_end : 2011-10-21T13:32:03`.
16. Try changing the separator to spaces or tabs and check that the 3 or
    4 columns of numbers are separated using that separator.
17. Change the dropdown to `ORSO Ascii (*.ort)`. The `Header` checkbox,
    separators and parameter settings are not applicable so they should
    be greyed out. The `Additional columns (includes Q resolution)`
    checkbox should be enabled.
18. Select a single `IvsQ_binned` workspace from the left list, e.g.
    `IvsQ_binned_11934`, and click Save. Open the `.ort` file that
    should have been created in your specified save directory. You
    should get a header at the top starting with
    `ORSO reflectivity data file`. There should be 8 columns of numbers
    with headings `Qz`, `R`, `sR`, `sQz`, `lambda`, `slambda`,
    `incident theta` and `sincident theta`.
19. Untick `Additional columns (includes Q resolution)` and re-save (the
    `Q resolution` checkbox should still be selected from the earlier
    steps). The file should now contain 4 columns of numbers with
    headings `Qz`, `R`, `sR` and `sQz`.
20. Untick `Q resolution` and re-save. The file should now contain 3
    columns of numbers with headings `Qz`, `R`, `sR`.
21. Change the dropdown to `ORSO Nexus (*.orb)`. The availability of the
    settings should be the same as they were for the
    `ORSO Ascii (*.ort)` format. Click Save and check that a file with
    extension `.orb` is saved out. This should be a Nexus file type, so
    will not be possible to view in a text editor. Use an HDF5 viewer to
    check that the contents of the file appear sensible.
22. Change the dropdown to `3 column (*.dat)`. All the settings should
    be greyed out as they are not applicable. Click Save to create the
    `.dat` file. You should get 3 columns of numbers separated by tabs
    (including a leading tab). At the top there is an integer indicating
    the number of lines in the data.
23. Change the dropdown to `ANSTO, MotoFit, 4 Column (*.txt)`. The
    settings remain greyed out. Click Save to create the `.txt` file.
    You should get 4 columns of numbers separated by tabs (with no
    leading tab).
24. Change the dropdown to `ILL Cosmos (*.mft)`. The settings remain
    greyed out apart from the parameters which should now be enabled.
    Click Save to create the `.mft` file. You should get 3 columns of
    numbers padded by spaces (including leading spaces). There should be
    a header starting `MFT` which includes any parameters you selected,
    the same as the Custom format.
25. In the Automatic Save section, tick the
    `Save automatically on completion` checkbox. This should enable the
    `Include individual row outputs for groups` checkbox. Try changing
    the selected file format - the
    `Save multiple datasets to a single file` checkbox should be
    disabled for all file formats apart from `ORSO Ascii (*.ort)` and
    `ORSO Nexus (*.orb)`.
26. With either the `ORSO Ascii (*.ort)` or `ORSO Nexus (*.orb)` format
    selected, untick the `Save automatically on completion` checkbox.
    This should disable the `Save multiple datasets to a single file`
    checkbox, as it is only applicable when auto-save is selected for
    one of the ORSO formats.
27. Try entering a non-existent or invalid save path and then try to
    Save. You should get an error saying that the path is invalid.

## Preview tab

1.  Go to the Reduction Preview tab.
2.  Type `INTER45455` into the `Run` input. Set the `Angle` to `1` and
    click `Load`. The instrument view plot should display the data on a
    detector with four banks. Note, with this dataset, we expect an
    error "Detector with ID..." to be thrown at this stage.
3.  Go to the drop-down underneath the color scale next to the second
    (slice viewer) plot and select `SymmetricLog10`. This should allow
    you to see the counts on the slice viewer plot more clearly. You
    should see what appear as roughly four horizontal lines of data on
    the plot.
4.  Going back to the instrument view plot, click the rectangle-select
    button above it and draw a single region that selects all detector
    banks. The selected detector segments should be summed and the
    result plotted on the slice viewer, appearing as a single line of
    data.
5.  Reduce the size of your original region on the instrument view and
    check that multiple regions can be added to the plot. Check that
    when moving and resizing regions, the slice viewer plot is updated.
6.  Check that you can delete regions from the instrument view by
    selecting them and pressing the delete key on your keyboard.
7.  Make sure you have at least one region selected on the instrument
    view.
8.  Click the rectangle select button above the slice viewer plot and
    draw a `Signal` region on the plot. A reduction will now be
    triggered for the selected spectra and the result plotted on the 1D
    plot.
9.  Click the drop-down on the rectangle select button and select
    `Transmission`. Draw a transmission region onto the slice viewer
    plot. Then, in the same way, add one or more `Background` regions.
    The reduction should be re-run each time a region is added:
    - You should see the tab quickly disable and re-enable.
    - Another run of `ReflectometryISISLoadAndProcess` will be logged in
      the Messages bar.
    - The 1D plot should update (although this is usually only
      noticeable when changes are made to the Signal region).
10. Check that moving and vertically resizing regions triggers a re-run
    of the reduction (note that changing only the width of the
    rectangular selectors on the slice viewer plot will **not** trigger
    a new reduction).
11. Check that you can delete one of the Background regions by selecting
    it and pressing the `Delete` key on your keyboard.
12. Click the `Apply` button at the bottom right of the tab. The
    selected regions of interest should be populated in the lookup table
    on the Experiment Settings tab.
13. Click the `Load` button to reload the same run. The slice viewer
    plot should display selectors for the regions of interest that you
    saved in the previous step. The instrument view plot currently does
    not do this so should not display any selectors. If you click
    `Apply` without making any changes then the `ROI Detector IDs` entry
    in the Experiment Settings lookup table should be unchanged (i.e. it
    shouldn't be cleared).
14. Back on the Reduction Preview tab, click the export button above the
    top left of the 1D plot. This should export a workspace called
    `preview_reduced_ws` to the ADS.
15. Right-click the workspace and select `Show History`:
    - In the Algorithms list, expand `ReflectometryISISLoadAndProcess`.
    - Click on `ReflectometryReductionOneAuto` and check in the right
      hand pane that the inputs for `ProcessingInstructions`,
      `BackgroundProcessingInstructions` and
      `TransmissionProcessingInstructions` correspond to the ranges of
      spectra you selected.
    - Expand `ReflectometryReductionOneAuto`.
    - Click on `ReflectometryISISSumBanks` and check that the input for
      `ROIDetectorIDs` matches the range of detector IDs you selected.
16. Back in the Reflectometry interface, go to the Runs tab. In the
    Process Runs table on the right-hand panel of the tab, enter Run
    `INTER45455` and Angle `1` into the first child row. Click Process.
17. Compare plots of the `preview_reduced_ws` (from the Preview
    reduction) with `IvsQ_binned_45455` (from the batch reduction). They
    should be the same.
18. The plots on the Reduction Preview tab are located within three
    dockable widgets. Check that the widgets can be undocked, re-docked,
    re-sized etc. without error or loss of functionality.
