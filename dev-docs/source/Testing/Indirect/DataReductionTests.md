# Indirect Data Reduction Testing

::: {.contents local=""}
:::

*Prerequisites*

- Download the [ISIS Sample Data](http://download.mantidproject.org)

## ISIS Calibration

*Preparation*

- Instrument `IRIS`
- You need access to the ISIS data archive

**Time required 2-5 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Data reduction`
2.  Make sure `Instrument` is set to `IRIS`
3.  Go to the `ISIS calibration`
4.  Enter `Input Runs` 26173
5.  Click `Run`
6.  This should generate a workspace with `_calib` at the end, delete
    this workspace
7.  Check the `Scale by factor` checkbox and change the value of the
    scale to 0.5.
8.  Click `Run`
9.  This should generate a workspace with `_calib` at the end
10. Make sure to keep the `_calib` workspace, it is needed for the
    `ISIS Diagnostics<isis_diagnostic_test>` test
11. Check the `Create RES` box
12. Click `Run`
13. This should generate a workspace with `_res` at the end
14. In the `Output` options, select the `_res` workspace and click
    `Plot Spectra`. This should produce a spectrum plot.
15. Then select the `_calib` workspace and use the down arrow to click
    `Plot Bins`. This should produce a bin plot.
16. Enter `Input Runs` 55878-55879 and check `Sum Files`
17. Click `Run`, this should produce a `_calib` workspace
18. Make sure to also keep this `_calib` workspace, it is needed for the
    next test
19. Enter `Input Runs` 59057-59059 and check `Sum Files`
20. Set `Reflection` to 004
21. Click `Run`
22. This should produce a new `_calib` workspace, with `004` in the
    name.
23. Before moving on, set `Reflection` to 002

## ISIS Energy Transfer

**Time required 5-10 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Data reduction`
2.  Make sure `Instrument` is set to `IRIS`
3.  Make sure the tab is set to `ISIS Energy Transfer`
4.  Check the `Sum Files` box
5.  In the `Input Runs` box enter `26184-26185`
6.  Click `Run`
7.  Check the `Use Calib File` box
8.  Change `File` to `Workspace` and choose the `_calib` workspace
    previously created (55878 from the previous test)
9.  Click `Run`
10. In the main GUI right-click on the
    `iris26184-26185_multi_graphite002_red` workspace
11. Choose `Plot spectrum`, note the number of spectra, should be 51.
    The spectra numbering should start from 0.
12. Click `Cancel`
13. In the `Data reduction` GUI, change the `Detector Grouping` to
    Groups
14. Set `Groups` to 5
15. Click `Run`
16. In the main GUI right-click on the
    `iris26184-26185_multi_graphite002_red` workspace
17. Choose `Plot spectrum`, note the number of spectra, should be 6. The
    spectra numbering should start from 0.
18. Choose `Plot All`, this should result in a plot of all 6 spectra
19. Open `Interfaces` \> `Inelastic` \> `Data Processor` and go to the
    `S(Q, W)` tab
20. Change `File` to `Workspace` and load the `_red` workspace just
    created
21. `Q-Low` and `Q-High` should be automatically updated to the y axis
    range of the contour plot.
22. `E-Low` and `E-High` should be automatically updated to the x axis
    range of the contour plot.
23. Click `Run`. An `_sqw` workspace should be created.
24. Check `Rebin in energy`
25. Click `Run`
26. Within the `Output` section, click the down arrow on the
    `Plot Spectra` button and then select `Open Slice Viewer`, this
    should open a slice viewer window.
27. Click `Manage User Directories` and set the default save location
28. Click `Save Result`
29. Check that the `.nxs` file was created in the correct location

## ISIS Diagnostics

**Time required 5-10 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Data reduction`
2.  With `Instrument` set to `IRIS`, navigate to the `ISIS Diagnostic`
    tab
3.  On `Input Runs`, type `26176` and press the Enter key
4.  The plot widget should render a spectrum with two black sliders
    indicating integration limits
5.  On the property browser, change the value of the `Preview Spectrum`
    property, the plot should update with the appropriate spectrum
    whenever the number is changed, unless is outside the range of
    allowed spectra
6.  Move the integration sliders, and check that the values of `Start`
    and `End` under the `Peak` property on the property browser are
    accordingly updated
7.  Change the values of `Start` and `End` on the property browser and
    check that the slider position is updated on the plot
8.  Tick the `Use Calibration` checkbox and select the workspace
    previously generated, `irs26173_graphite002_calib`, on the data
    selector
9.  Change the `Spectra Min` property to 3
10. Click `Run`
11. A preview spectra should be rendered, plotting the integrated counts
    versus the spectrum number.
12. A new workspace with suffix `_slice` should be generated on the ADS
13. On Output, clicking the button `Plot Spectra` should open a new plot
    window with the same data as the preview plot.
14. Tick on the `Use Two Ranges` property. Two green sliders should
    appear on the plot
15. Move the green sliders and check that the `Start` and `End`
    properties under the `Background` property are updated accordingly
16. Selecting a non-overlapping background range, click on the `Run`
    button
17. The preview plot and the workspace ending in `_slice` should update
    with the new integrated time slice

## Transmission

**Time required 3-5 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Data reduction`
2.  Make sure `Instrument` is set to `IRIS`, `Analyser` to `graphite`
    and `Reflection` to `002`
3.  Navigate to the `Transmission` tab
4.  On `Sample`, type `26176` and press the Enter key
5.  On `Background`, type `26174` and press the Enter key
6.  Click `Run`
7.  The preview plot should be rendered, displaying three lines
    indicating the monitor detection for the can, the sample, and the
    transmission
8.  A workspace with suffix
    <span class="title-ref">\_transmission</span>, and a workspace group
    with suffix <span class="title-ref">\_transmission_group</span>
    should be generated on the ADS. The
    <span class="title-ref">\_transmission_group</span> workspace should
    contain three workspaces with suffices
    <span class="title-ref">\_Sam</span>,
    <span class="title-ref">\_Can</span> and
    <span class="title-ref">\_Trans</span>
9.  Clicking the `Plot Spectra` button, a plot window should prompt,
    displaying the same lines as in the preview plot
