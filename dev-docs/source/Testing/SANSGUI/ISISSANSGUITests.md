# SANS GUI Testing

```{contents}
:local:
```

## Data reduction

**Time required: about 1 hour**

------------------------------------------------------------------------

### Set up

1.  Get the Training data from the downloads page on the Mantid Project
    website.
2.  Open `Interfaces` \> `SANS` \> `ISIS SANS`.
3.  Click `Manage Directories`. This opens Mantid's main dialog for
    managing search paths. Ensure the Training data directory is in the
    search directories.
4.  If you haven't set one up yet, add a folder to save test data into
    later.
5.  Set the default save directory in `Manage Directories`
6.  Click OK on the manage-directories dialog and ensure the `Save Path`
    on the SANS GUI (below batch file) displays the correct save path.
7.  Click `Load User File`; from the Training data in the `loqdemo`
    folder, choose `MaskFile.toml`.
8.  Click `Load Batch File`; from the Training data in the `loqdemo`
    folder, choose `batch_mode_reduction.csv`.

### Automatic Save Selection

1.  Select `File` or `Both` from the `Save Options` at the bottom right
    of the screen.
2.  In the `Reduction` section to the left of the `Save Options`, switch
    between the `1D` and `2D` radio buttons..
    - When 1D is selected, `CanSAS (1D)` and `NxCanSAS (1D/2D)` should
      be checked.
    - When 2D is selected, only `NxCanSAS (1D/2D)` should be checked.
3.  Check `RKH (1D/2D)`.
4.  Change the selected `Reduction` radio button.
5.  The options should revert to the defaults above (with `RKH (1D/2D)`
    unchecked).
6.  Select `Memory`. The `CanSAS (1D)`, `NxCanSAS (1D/2D)`, and
    `RKH (1D/2D)` checkboxes should be disabled.
7.  Swap between `Memory` and `File` with a `2D` reduction mode.
    `CanSAS (1D)` should always stay disabled.
8.  Set the `Save Option` back to `Memory` to continue with the rest of
    the tests.

### Runs table editing

In the `Runs` tab:

1.  Check that the `Insert`, `Delete`, `Copy`, `Paste`, `Cut` and
    `Erase` icons work as expected on table rows.
2.  Tick the `Sample Geometry` button - some extra columns should
    appear.
3.  Create multiple rows in the table with different data e.g. one row
    with an output name, one row without.
4.  Try changing a random setting in the settings tab and remember what
    you set it to.
5.  Make some more edits to the table. The settings will be reverted to
    the defaults set in the User File.
6.  Click the `Export Table` button and save the table as a csv file.
    Check the file in an editor or Excel and ensure it looks like a
    sensible representation of the table in the format
    `key,value,key,value,...`. All columns except `Options` and
    `Sample Shape` should be included.
7.  Try unticking `Sample Geometry` and ticking `Multi-period` and
    resave the CSV. The displayed columns change but the saved file
    should contain the same set of columns regardless of whether these
    are ticked.
8.  Click `Load Batch file` and select the newly saved table. All
    columns that were saved should be loaded.
9.  Try deleting and/or reordering some of the columns in the saved file
    and re-load it. All of the values in the file should be populated in
    the correct columns.
10. Re-load the original batch file.

### User files

1.  Change some values on the `Settings` tab and make a note of what you
    changed.
2.  Re-load the user file and check the values you changed - they should
    have reverted to their original values.
3.  Change some values on the `Beam Centre` tab. Re-load the user file.
    The inputs in the `Centre Position` section should revert to their
    original values. The inputs in the `Options` section (such as the
    radius limits) should not revert.
4.  Ensure that you can load the old style `MaskFile.txt` user file from
    the sample data.
    - **Note:** In order to see this file, you may need to change the
      settings in the file browser window to look for `.txt` files
      instead of `.TOML` files.
5.  In the table on the `Runs` tab, under the `User File` column, enter
    `MaskFile.toml` in one row and `MaskFile.txt` in the other row.
    Click `Process All`. After some seconds, the rows should turn green
    to indicate that they processed successfully.
6.  Re-load the original user and batch files as per the set-up
    instructions.

### Display mask

In the `Settings` tab:

1.  Go to `Mask`.
2.  Click `Display Mask`.
3.  This should give an instrument view with a circle at the centre.
4.  Close the Instrument View window
5.  Go to `Q, Wavelength, Detector Limits` sub-tab.
6.  Change the `Phi Limit` to read 0 to 45 and uncheck
    `use mirror sector`.
7.  Go to `Mask` sub-tab.
8.  Click `Display Mask`.
9.  This should give an instrument view where only angles 0-45 are
    unmasked.
10. Change the settings back to -90 to 90 and reselect
    `use mirror sector`.

### Processing

*1D reduction*

1.  Clear all workspaces in your workspaces list if they are not empty.
2.  In `General, Scale, Event Slice, Sample` sub-tab, ensure the
    `Reduction Mode` is `All`.
3.  In the `Runs` tab, under `Save Options`, select `Both`, and tick
    `CanSAS (1D)` and `NXcanSAS (1D/2D)`.
4.  Select `Save Can`.
5.  Click `Process All`.
6.  After some seconds the rows should turn green.
7.  In the workspaces list, there should be a series of new workspaces;
    five group workspaces and four 1D workspaces.
8.  Check your default save directory. For each reduction two banks
    (HAB/main) should be saved. In total, there should be 20 workspaces
    saved. For each row, file type, and bank, there should be a reduced
    file (with no suffix) and a `sample` file. The `first_time` line
    should also produce a `can` workspace for each file type and bank.
    This is because both workspaces have the same `can` input run
    numbers and so the reduction only calculates it once.
9.  Double-click the 1D workspaces and you should get a single line
    plot.
10. Clear the newly created files and workspaces to make the next test
    easier
11. Change the contents of the first cell in the first row to `74045`
    and click `Process Selected`.
12. The row should turn blue; hovering over the row should give an error
    message.
13. Change the first column of the first row back to `74044`.
14. Click on another row, the modified row should have cleared its
    colour

*2D reduction*

1.  Clear all workspaces in your workspaces list if they are not empty.
2.  In `General, Scale, Event Slice, Sample` sub-tab, ensure the
    `Reduction Mode` is `All`.
3.  Switch to the 2D `Reduction Mode`.
4.  Click `Process All`.
5.  You should get four 2D workspaces instead of the previous 1D
    workspaces (they will have 100 spectra instead of 1). Double-click
    them and check you can do a colourfill plot.
6.  Check your save directory. There should now only be a `.h5` file for
    each output.
7.  Clear the newly created files and workspaces to make future tests
    easier
8.  Change `Reduction` back to 1D.
9.  Click `Process All`.
10. When it completes, Check the `Multi-period` box - six additional
    columns should appear in the table.
11. Delete all rows and re-load the batch file.

*Merged reduction*

1.  In the `Settings` tab, `General, Scale, Event Slice, Sample`
    sub-tab, set `Reduction Mode` to `Merged`.
2.  Return to the `Runs` tab.
3.  Ensure save outputs `CanSAS (1D)` and `NXcanSAS (1D/2D)` are ticked.
4.  Click `Process All`.
5.  The workspaces list should now contain a group named
    `LAB_and_HAB_workspaces_from_merged_reduction` that contains the
    `main` and `HAB` workspaces, which were previously ungrouped for a
    non-merged reduction.
6.  Check your save directory. As well as the previous 1D outputs, there
    should now be an additional `.xml` and `.h5` output file for the
    merged output for each row.
7.  In the `Settings` tab, `General, Scale, Event Slice, Sample`
    sub-tab, change the `Reduction Mode` back to `All`.

*Scaled Background Subtracted Reduction*

1.  Create a new copy of the User File in your file browser.
2.  Open this new copy in a text editor and find the
    `[detector.configuration]` section.
3.  Under this section, make sure setting `selected_detector` is set to
    `Merged`.
4.  Back in the ISIS SANS interface, change the user file to this new
    file.
5.  Click over to the `Runs` tab.
6.  Set the `Save Options` to `Memory`.
7.  Select one of the rows and click `Process Selected`
8.  Take note of the name of the reduced workspace with `merged` in the
    title.
9.  Make a copy of the row you just processed using the `Copy` and
    `Paste` buttons above the runs table.
10. Change the `Output Name` of the new row to something like
    `bgsub_test`.
11. Check the `Scaled Background Subtraction` checkbox.
12. In the `BackgroundWorkspace` column, enter the name of the merged
    workspace you took note of before.
13. In the `ScaleFactor` column, enter `0.9`.
14. Set the `Save Options` to `Both` and ensure that save outputs
    `CanSAS (1D)` and `NXcanSAS (1D/2D)` are ticked.
15. Select this new row and click `Process Selected`.
16. When it completes, two output files should have been created with
    `bgsub_test` in the name. One, which is the normal output data.
    Another with the scaled subtraction, which should have `_bgsub`
    appended to the name.
17. Right click on each of these and select `Show Data`. The subtracted
    workspace's values should be 10% of the of the unsubtracted
    workspace's values.
18. Check that your save location contains files for both the background
    subtracted workspace and the normal reduction output.

### Save Other

*Single Workspace*

1.  Navigate to the `Runs` tab, making sure there are some reduced
    workspaces present in the ADS. Follow one of the "Processing"
    instruction sets above if you need to create some.
2.  Click the `Save Other` button.
3.  Select one of the workspaces from the list.
4.  Provide a path to a new save directory, and provide a file name.
5.  Click `Save`.
6.  Check the file was saved to the correct location on your system.

*Multiple Workspaces*

1.  Select multiple workspaces with Shift or Ctrl/Cmd.
2.  Provide a suffix for the files.
3.  Click `Save`.
4.  Check that the files were saved with their workspace's names, but
    with the provided suffix appended.

### Beam centre finder

In the `Beam centre` tab:

1.  Make a note of the four values representing the rear/front detector
    centre positions.
2.  Check that the `Find Rear Centre` radio button is selected and click
    run.
3.  A plot should appear - make sure to show it if it is behind another
    window. It should be updated with four lines, which gradually get
    closer together. This might take a while to run.
4.  Check the values in the first two text boxes at the top (Centre
    Position - Rear) have changed when it has finished running.
5.  Select the `Find Front Centre` radio button and re-run the test.
6.  Four more lines should appear on the same plot. This time, in the
    values at the top, only the values for the front should have
    changed.

### Sum runs

In the `Sum Runs` tab:

1.  Enter `74044, 74019` in the top line.
2.  Click `Add` at the side.
3.  Check that `LOQ74044-add` is automatically entered as the Save File
    at the bottom of the tab.
4.  At the top-right of the tab, click the `Select Save Directory`
    button and select a directory in your managed paths.
5.  Click `Sum` at the bottom.
6.  Go back to the `Runs` tab.
7.  Remove all rows.
8.  Reload the batch file as before.
9.  Change the first column of both rows to `LOQ74044-add`.
10. Click `Process All`.
11. This should now process as before.

### Diagnostics

In the `Diagnostic Page` tab:

1.  For run choose `Browse` and load the `LOQ74044.nxs` file.
2.  Click each of the `Integral` buttons.
3.  They should produce plots.
4.  Check the `Apply Mask` boxes and click the buttons again.
5.  They should produce new, slightly different plots.

### Display

1.  In the `Runs` tab, check that all table, process, and load buttons
    have clear tooltips by hovering over them.
2.  Check that `Zero Error Free` and `Use Optimizations` have clear
    tooltips.
3.  In the settings, hover over a random selection of buttons and text
    boxes to check tooltips are still there. Users rely on the tooltips
    a lot and really do notice each missing one. *Note: The*
    `Wavelength` *section of the settings is missing its tooltips. We
    and the users are aware of this so an issue should not be made when
    it is discovered.*
