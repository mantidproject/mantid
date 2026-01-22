# Inelastic Data Processor Testing

```{contents}
:local:
```

*Prerequisites*

- Download the [ISIS Sample Data](http://download.mantidproject.org)
- Make sure that the data set is on your list of search directories

## Symmetrise tab

**Time required 5-10 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Symmetrise` tab
3.  On the File Input, click on `Browse`, a dialog window should prompt.
4.  Find the file `irs26176_graphite002_red.nxs` from the ISIS Sample
    Data and Load it.
5.  The first spectrum should be rendered in the plot browser.
6.  On the plot, drag and move the rightmost vertical green slider to be
    approximately at 0.4 on the x-axis, the value of `Ehigh` should
    change accordingly.
7.  Move the other slider to be approximately at 0.2 on the x-axis, the
    value of `Elow` should change accordingly.
8.  Click on the `Preview` button. The symmetrised plot should be
    rendered on the preview graph.
9.  Click the `Run` button.
10. A Workspace named `irs26176_graphite002_sym_pn_red` should be
    created on the ADS.
11. On the Output widget, click on `Plot Spectra` button. A new plot
    window should appear with the same plot as the preview.
12. On the Symmetrise Property Browser: Change the value of `Elow` and
    `Ehigh` properties and see that the vertical green sliders. respond
    accordingly, but only when the input values are within range. It
    shouldn't be possible to set `Elow` to the right of `Ehigh` and
    viceversa. At the same time, it shouldn't be possible to enter
    negative numbers, or positive numbers larger than the maximum data
    range.
13. Change the value of `Spectrum No` property to 5 and see that the
    spectra on the plot is updated.
14. Change the `ReflectType` property from `Positive to Negative` to
    `Negative to Positive`, the sliders should move to the negative
    range of the spectra.
15. Now `Ehigh` should be closer to 0 than `Elow`. Repeat checks of step
    12, noting that only negative inputs can be entered now.
16. Set `Ehigh` to -0.1 and `Elow` to -0.3 and click on the `Preview`
    button. The symmetrise plot should be rendered on the preview graph.
    The axis data range must go from -0.3 to 0.3.
17. Click on the `Run` button.
18. A Workspace named `irs26176_graphite002_sym_np_red` should be
    created on the ADS.
19. On the ADS, select `irs26176_graphite002_sym_np_red` and right-click
    to select `Plot -> Spectrum...`. On `Spectrum Numbers` select 5.
    Check that the spectrum is the same as the one in the `Preview` plot
    of the symmetrise interface.

## $S(Q, \omega)$ tab

**Time required 10-15 minutes**

------------------------------------------------------------------------

### 1. ISIS data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `S(Q,w)` tab
3.  On the File Input, click on `Browse`, a dialog window should prompt.
4.  Find the file `irs26176_graphite002_red.nxs` from the ISIS Sample
    Data and Load it.
5.  The data will be plot as a contour plot in the interface.
6.  Set <span class="title-ref">Q Low</span>, <span class="title-ref">Q
    Width</span> and <span class="title-ref">Q High</span> to be 0.45,
    0.05 and 1, respectively.
7.  Click <span class="title-ref">Run</span> button.
8.  There should be two new workspaces in the ADS with the prefixes
    `_sqw` and `_rqw`.
9.  On <span class="title-ref">Output</span> options of the interface.
    Select from the drop-down menu on the right of the `Plot Spectra`
    button the option to `Open Slice Viewer` and click.
10. A `Slice Viewer` window should prompt, check on the `Slice Viewer`
    that the spectra slices range in q-values from 0.45 to 1.
11. Back on the interface, check <span class="title-ref">Rebin in
    Energy</span> and change `E Low` and `E High` to be -0.2 and 0.2.
12. Click <span class="title-ref">Run</span>. A new rebinned workspace
    with suffix <span class="title-ref">\_r</span> should appear on the
    ADS.
13. On <span class="title-ref">Output</span> options of the interface.
    Select from the drop-down menu on the right of the `Plot Spectra`
    button the option to `Plot 3D Surface` and click.
14. The resulting 3D plot should range in the Energy Transfer axis from
    -0.2 to 0.2 $meV$.
15. Don't remove yet the workspace with suffix `_sqw` from the ADS as
    you will use it for the Moments interface test.
16. Repeat instructions 4 to 10, but loading instead the
    `MAR27691_red.nxs` file from the ISIS Sample Data set.

### 2. ILL data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `S(Q,w)` tab
3.  Open the Settings widget from the bottom left of the interface, and
    untick <span class="title-ref">Restrict allowed input files by name
    (recommended)</span>
4.  On the File Input, click on `Browse`, a dialog window should prompt.
5.  Find the file `243489-243506ElwinHeat_H2O-HSA-stw.nxs` from the
    SystemTest data directory and Load it.
6.  The data will be plot as a contour plot in the interface.
7.  The <span class="title-ref">Q Low</span>, <span class="title-ref">Q
    Width</span> and <span class="title-ref">Q High</span> will be
    automatically set to 0.2, 0.05 and 2.25 respectively.
8.  Click <span class="title-ref">Run</span> button.
9.  There should be two new workspaces in the ADS with the prefixes
    `_sqw` and `_rqw`.
10. Open the Settings widget from the bottom left of the interface, and
    tick <span class="title-ref">Restrict allowed input files by name
    (recommended)</span>

## Moments tab

**Time required 2-3 minutes**

------------------------------------------------------------------------

### 1. Indirect data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Moments` tab
3.  On the File Input, change the drop-down selector from `File` to
    `Workspace`.
4.  Select the previously generated workspace `irs26176_graphite002_sqw`
    from the selector.
5.  First spectrum should be rendered on the plot widget, with two
    vertical sliders placed on the integration limits, the limits on the
    sliders and on the property browser should match.
6.  Change the limits by moving the sliders to be at -0.2 and 0.2,
    respectively. On the property browser, `EMin` and `EMax` should be
    updated with the new limits.
7.  Click the `Run` button.
8.  The Preview plot should be updated with a representation of the
    first three moments with respect to Q. A workspace with the suffix
    <span class="title-ref">\_Moments</span> should be generated on the
    ADS.
9.  On the Output edit, the name of the output workspace should be
    greyed out. The spectra numbers box should also be disabled, and
    should read <span class="title-ref">0,2,4</span>. Click on the
    `Plot Spectra` button. A plot window should be generated with three
    of the calculated moments.

### 2. Invalid data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Moments` tab
3.  On the File Input, click on `Browse`, a dialog window should prompt.
4.  Find the file `MAR27698_red.nxs` from the ISIS Sample Data and Load
    it.
5.  Click the `Run` button.
6.  There should be a red error message in the logger. No data should be
    plotted in the bottom embedded plot.

## Elwin tab

**Time required 3 - 5 minutes for each**

------------------------------------------------------------------------

### 1. Direct data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Elwin` tab
3.  Click on `Add Workspaces`, a dialog window should prompt.
4.  Enter `MAR27691_red.nxs` in `Input file`. The table of the dialog
    should be populated with the `MAR2791_red` workspace.
5.  Select the workspace from the table and click on `Add Data`. Close
    the dialog.
6.  Back on `Elwin` tab, click `Run` - this should produce 3 new
    workspaces `_elf`, `_eq` and `_eq2`
7.  Open the `Add Workspaces` dialog again, and in `Input file` choose
    `browse`. Navigate to the ISIS-Sample data and select the two files,
    `MAR27691_red.nxs` and `MAR27698_red.nxs` using shift key.
8.  Add the loaded workspaces
9.  Click `Run`
10. This should result in three new workspaces again, this time with
    file ranges as their name
11. In the main GUI right-click on `MAR27691-27698_red_elwin_eq2` and
    choose `Plot Spectrum`, choose `Plot All`
12. This should plot two lines of $ln(Counts(microAmp.hour))^{-1}$ vs
    $Q2$

### 2. Indirect data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Click on `Add Workspaces`, a dialog window should prompt
3.  Enter `irs26174_graphite002_red.nxs` in `Input file`. The table of
    the dialog should be populated with the `irs26174_graphite002_red`
    workspace.
4.  Select the workspace from the table and click on `Add Data`. Close
    the dialog.
5.  Back on `Elwin` tab, click `Run` - this should produce 3 new
    workspaces `_elf`, `_eq` and `_eq2`
6.  Right-click on the `irs26174_graphite002_red_elwin_eq` workspace and
    `Save Nexus`; save to a location of your choice. **NB** keep this
    workspace if you are doing the
    `QENS Fitting Manual Test <inelastic_qens_fitting_testing>`
7.  Remove the Workspace from the interface by clicking on the
    `Select All` button and then the `Remove Selected` button.
8.  Open the `Add Workspaces` dialog again, there should be a
    `irs26174_graphite002_red` workspace on the table with `Ws Index`
    equal to `0-50`.
9.  In `Input file` choose `Browse`. Navigate to the ISIS-Sample data
    folder and select the file `irs26176_graphite002_red.nxs`. A new
    entry, `irs26176_graphite002_red`, should be added to the data
    table, with `Ws Index` equal to `0-50`. (beware that there is a
    `irs26176_graphite002_red.nxs` file on the `Usage Data` set. This is
    NOT the correct file for this test, it should be loaded from the
    Sample Data-ISIS set)
10. With both workspaces on the data table, click on the `Select All`
    button and then on the `Add Data` button. Both workspaces should be
    added to the `Elwin` interface.
11. Change the integration range from -0.2 to 0.2
12. Click `Run`
13. This should result in three new workspaces again, this time with
    file ranges as their name.
14. In the main GUI right-click on
    `irs26174-26176_graphite002_red_elwin_eq2` and choose
    `Plot Spectrum`, choose `Plot All`
15. This should plot two lines of $ln((meV))^{-1}$ vs $Q2$

## I(Q, t) tab

**Time required 3 - 5 minutes for each**

------------------------------------------------------------------------

### 1. Direct data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Iqt` tab
3.  Load the `MARI27691_sqw.nxs` file from the sample data
4.  Load the resolution file `MARI27698_sqw.nxs` from the sample data
5.  Click `Run`
6.  A new workspace with the suffix `_iqt` should appear in the main
    GUI, it should be a workspace with 17 histograms and 3 bins.
7.  Click `Plot Current preview` this should plot the same data as the
    preview window
8.  Choose some workspace indices (e.g. 0-2) in the `Output` section and
    click `Plot Spectra` this should give a plot with the title
    *MARI27691_iqt*
9.  Click the down arrow on the `Plot Spectra` button and then select
    `Plot Tiled`. This should give a tiled plot of the selected
    workspace indices.

### 2. Indirect data

1.  Go to `Interfaces` \> `Inelastic` \> `Data Processor`
2.  Go to the `Iqt` tab
3.  Load the `irs26176_graphite002_red.nxs` file from the sample data
4.  Load the resolution file `irs26173_graphite002_res.nxs` from the
    sample data
5.  Click `Run`
6.  A new workspace with the suffix `_iqt` should appear in the main
    GUI, it should be a workspace with 51 histograms and 86 bins. **NB**
    keep this workspace if you are doing the
    `QENS Fitting Manual Test <inelastic_qens_fitting_testing>`
7.  Click `Plot Current preview` this should plot the same data as the
    preview window
8.  Choose some workspace indices (e.g. 0-2) in the `Output` section and
    click `Plot Spectra` this should give a plot with the title
    *irs26176_graphite002_iqt*
9.  Click the down arrow on the `Plot Spectra` button and then select
    `Plot Tiled`. This should give a tiled plot of the selected
    workspace indices.
