# Filter Events Interface Testing

::: {.contents local=""}
:::

*Prerequisites*

- Download the <span class="title-ref">Usage Examples
  \<https://www.mantidproject.org/installation/index#sample-data\></span>

## Filter Events

**Time required: 10-15 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Utility` \> `Filter Events`
2.  Browse for file 'CNCS_7860_Event.nxs' from the Usage Data set.
3.  Click `Load`. After a few seconds, plot should be updated with a
    graph of summed up Counts vs.Time.
4.  `_Summed_..` workspace should be generated on the ADS, as well as
    `CNCS_7860_Event` workspace.
5.  Clicking on the `Refresh` button should update the drop-down list
    with the name of the loaded workspace.
6.  Check the vertical dashed green range markers on the plot work
    properly when dragging: `Starting Time` and `Stopping Time` Text
    Edits are updated accordingly.
7.  Check the horizontal dot-dashed blue range markers on the plot work
    properly when dragging: `Minimum Value` and `Maximum Value` Text
    Edits are updated accordingly.
8.  Check that the Text Edits for `Starting Time` and `Stopping Time`
    (similarly with `Minimum Value` and `Maximum Value`) work correctly:
    - Position of markers on the plot are updated appropriately when
      changing the numerical value of the edits and pressing
      <span class="title-ref">Enter</span>.
    - Setting a value on `Starting Time` larger than the current value
      on `Stopping Time` is not allowed, and vice versa.
    - Data Validation is working, you can't set non-numeric characters
      on the edits.
9.  Moving the mouse on the plot updates a label in the top left corner
    with the <span class="title-ref">(x,y)</span> position on the graph.
10. On `Output Name` write <span class="title-ref">FilteredTemp</span>.
11. On `Sample Log` drop-down menu select `SampleTemp` and hit on `Plot`
    button. Plot should update with the temperature log for the run vs.
    time.
12. Move the upper vertical range marker to be at approximately `279.95`
    degrees and the lower vertical range marker to be at approximately
    `279.91` degrees.
13. Click on `Filter` button. That should generate several workspaces in
    the ADS. Two table workspaces ending with `_info`, `_splitters` and
    a group workspace named `FilteredTemp` containing one event
    workspace named `FilteredTemp_0`, as well as a `TOFCorrTable` 2D
    workspace.
14. Right-click on `FilteredTemp_0` and select `Show Sample Logs`. On
    Sample Log Window, check the `SampleTemp` entry and make sure the
    temperature range is approximately the same as selected with the
    markers in the interface.
15. Back to the filter events interface, click on the `Refresh` button.
    The drop-down list should refresh with the available event
    workspaces on the ADS (`CNCS_7860_Event` and `FilteredTemp_0`).
16. Select `FilteredTemp_0` and click on `Use` button. Plot should
    update accordingly.
17. On output name write <span class="title-ref">FilteredTime</span>, on
    `Starting Time` text edit write `80` and then press `Enter`. On
    `Stopping Time` text edit write `100` and press `Enter`.
18. Select `Filtered by Time` tab and on `Time Interval` text edit write
    `10`. Then click on `Filter` button. Two table workspaces ending
    with `_info`, `_splitters` and a group workspace named
    `FilteredTime` containing two event workspaces named
    `FilteredTime_0` and `FilteredTime_1` appear on ADS.
19. Double-click on `FilteredTemp_0_info` table workspace. The table
    should contain two rows, indicating two time intervals of
    approximately 10 seconds each, and the corresponding workspace group
    index.
20. Back to the filter events interface, without changing anything else
    from previous interaction, go to `Advanced Setup` tab and tick the
    `Fast Log` checkbox. Go back to `Filter by Time` and click on
    `Filter` button again. Workspace ending in `_splitters` should be a
    2D workspace, all other workspaces should remain the same.
21. Now on `Algorithms` select `FilterEvents` algorithm and click on
    `Execute`. As `InputWorkspace` select `FilteredTemp_0` workspace. On
    Splitter workspace select `FilteredTemp_0_splitters`, and on
    `Information Workspace` select `FilteredTemp_0_info`. Tick on
    `GroupWorkspaces` checkbox. And name the `OutputWorkspaceBaseName`
    as `FilteredTimeRedux`.
22. `Run` the algorithm with `Group Workspaces` checked. On the ADS, a
    new group event workspace named `FilteredTimeRedux` should appear.
    The two event workspaces that it contains should be equal to the two
    event workspace previously generated through the interface,
    `FilteredTime_0` and `FilteredTime_1`.
23. Back to the Filter Events interface. Click on `Refresh` button
    again, and make sure it updates with the list of all event
    workspaces on the ADS.
