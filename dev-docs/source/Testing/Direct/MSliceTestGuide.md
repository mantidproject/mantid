> $\renewcommand\AA{\mathring{A}}$

# MSlice Testing

```{contents}
:local:
```

## Introduction

MSlice is a tool for visualizing cuts and slices of inelastic neutron
scattering data. This version uses Mantid to process the data and plots
it using matplotlib. It includes both a GUI and a commandline interface
with a script generator.

See here for the current MSlice documentation:
<http://mantidproject.github.io/mslice>

## Set Up

1.  Ensure you have the [ISIS Sample
    Data](http://download.mantidproject.org) available on your machine.
2.  If you are using a conda install of Mantid Workbench, make sure to
    install MSlice by running `mamba install -c mantid mslice` inside
    your conda enviroment and restart the Mantid Workbench.
3.  If you are using a standalone install of Mantid Workbench, MSlice
    should already be available within it.
4.  Open `Interfaces` \> `Direct` \> `MSlice`
5.  Go to the `Data Loading` tab and select `MAR21335_Ei60meV.nxs` from
    the sample data.
6.  Click `Load Data`
7.  This should open the `Workspace Manager` tab with a workspace called
    `MAR21335_Ei60meV`

## Default Settings

1.  In the `Options` menu, change `Default Energy Units` from `meV` to
    `cm-1` and `Cut algorithm default` from `Rebin (Averages Counts)` to
    `Integration (Sum Counts)`.
2.  The `en` setting on the `Slice` tab changes from `meV` to `cm-1` and
    the values in the row labelled `y` change. Please note that the
    `Slice` and `Cut` tabs are not enabled before at least one data set
    has been loaded.
3.  Navigate to the `Cut` tab
4.  Verify that `en` is set to `cm-1` and `Cut Algorithm` to
    `Integration (Sum Counts)`
5.  Change both settings back to their original values,
    `Default Energy Units` to `meV` and `Cut algorithm default` to
    `Rebin (Averages Counts)`.

## Taking Slices

### 1. Plotting a Slice

1.  In the `Workspace Manager` tab select the workspace
    `MAR21335_Ei60meV`
2.  Click `Display` in the `Slice` tab without changing the default
    values
3.  On the slice plot, click `Keep`

<figure>
<img src="../../../../docs/source/images/slice_plot.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/slice_plot.png" />
</figure>

### 2. Modifying a Slice

1.  Modify the slice settings in the `Slice` tab, for instance the
    values for x for `from` to `1.5` and `to` to `5.5` , and click
    `Display`
2.  A second slice plot should open with a plot reflecting your changes
    in the settings
3.  The original slice plot should remain unchanged

<figure>
<img src="../../../../docs/source/images/modified_slice_plot.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/modified_slice_plot.png" />
</figure>

### 3. The Plots Tab

1.  Navigate to the `Plots` tab of MSlice and check that there are
    entries for two plots
2.  Open the `Plots` tab of Mantid and check that there are no entries
    for plots
3.  Select one of the plots in the `Plots` tab of MSlice and click on
    `Hide`, the corresponding plot should disappear
4.  Now click on `Show` for this plot and it should re-appear again
5.  Double-click on elements of the original slice plot and modify
    settings, for instance the plot itself and the colorbar axes
6.  Change the plot title and the y axis label to LaTeX, for instance
    `$\mathrm{\AA}^{-1}$`, and ensure the text is displayed correctly
    (for `$\mathrm{\AA}^{-1}$` it should be $\mathrm{\AA}^{-1}$)
7.  Ensure that the slice plot changes accordingly
8.  Click `Make Current` on the original slice plot
9.  Modify the slice settings in the `Slice` tab again and click
    `Display`
10. This time the new slice plot overwrites the original slice plot

### 4. Overplot Recoil Lines and Bragg Peaks

1.  Navigate to the `Information` menu on the slice plot
2.  Select `Hydrogen` from the submenu for `Recoil lines`. A blue line
    should appear on the slice plot.
3.  Select two or three materials from the submenu for `Bragg peaks` and
    ensure that Bragg peaks in different colours per material are
    plotted on the slice plot.
4.  Make sure that when deselecting one of the materials only the
    respective Bragg peaks are removed from the slice plot but the ones
    still selected remain.

<figure>
<img src="../../../../docs/source/images/recoil_line_bragg_peaks.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/recoil_line_bragg_peaks.png" />
</figure>

### 5. Generate a Script

1.  Navigate to the `File` menu on the slice plot
2.  Select `Generate Script to Clipboard` and paste the script into the
    Mantid editor. Please note that on Linux `Ctrl + V` might not work
    as expected. Use `shift insert` instead in this case.
3.  Run the script and check that the same slice plot is displayed

## Taking Cuts

### 1. Plotting a Cut

1.  In the `Workspace Manager` tab select the workspace
    `MAR21335_Ei60meV`
2.  Navigate to the `Cut` tab
3.  In the row labelled `along`, set the `from` value to `0` and the
    `to` value to `10`
4.  In the row labelled `over`, set the `from` value to `-5` and the
    `to` value to `5`
5.  Click `Plot`. A new window with a cut plot should open.

<figure>
<img src="../../../../docs/source/images/cut_q.png" class="align-center"
style="width:80.0%" alt="../../../../docs/source/images/cut_q.png" />
</figure>

### 2. Changing the intensity of a Cut

1.  Navigate to the `Intensity` menu on the cut plot
2.  Select `Chi''(Q,E)` and set a value of `100`
3.  The y axis of the cut plot should change to a higher maximal value

<figure>
<img src="../../../../docs/source/images/cut_q_chi.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/cut_q_chi.png" />
</figure>

### 3. Modifying a Cut

1.  Check that the menu item `Recoil lines` is disabled within the menu
    item `Information`.
2.  Modify the step size on the `Cut` tab to `0.02` and click
    `Plot Over`. A second cut should appear on the cut plot in a
    different colour.
3.  Click on Plot Options on the cut plot and modify settings
4.  Ensure that the cut plot changes accordingly
5.  Click on Save to Workbench on the `Cut` tab and check that in Mantid
    a workspace with the name `MAR21335_Ei60meV_cut(-5.000,5.000)`
    appears. In case there are several cuts with the same parameter,
    these workspaces are differentiated by appending an index, so there
    might be a workspace named `MAR21335_Ei60meV_cut(-5.000,5.000)_(2)`
    instead.
6.  In the row labelled `over`, set the `from` value to `-1` and the
    `to` value to `1` and click `Plot`
7.  Navigate to the tab `MD Histo` tab and check that there are at least
    two entries, `MAR21335_Ei60meV_cut(-5.000,5.000)` and
    `MAR21335_Ei60meV_cut(-1.000,1.000)`. Please note that there might
    be more entries from the previous tests.
8.  Select `MAR21335_Ei60meV_cut(-1.000,1.000)` and click
    `Save to Workbench`
9.  Check that in Mantid a workspace with the name
    `MAR21335_Ei60meV_cut(-1.000,1.000)` appears
10. Navigate to the `Cut` tab
11. In the row labelled `along`, select `DeltaE`
12. In the row labelled `over`, select `2Theta`
13. In the row labelled `along`, set the `from` value to `-5` and the
    `to` value to `5`
14. In the row labelled `over`, set the `from` value to `30` and the
    `to` value to `60`
15. Click `Plot`
16. Depending on the cutting algorithm, the plot will look similar to
    the plot below when using the integration method

<figure>
<img src="../../../../docs/source/images/cut_plot.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/cut_plot.png" />
</figure>

and similar to the plot below when using the rebin method.

<figure>
<img src="../../../../docs/source/images/cut_plot_2.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/cut_plot_2.png" />
</figure>

### 4. Interactive Cuts

1.  Navigate to the `Slice` tab of the `Workspace Manager` tab
2.  Click `Display` in the `Slice` tab without changing the default
    values
3.  On the slice plot, select `Interactive Cuts`
4.  Use the cursor to select a rectangular region in the slice plot. A
    second window with a cut plot should open.
5.  Check that the menu item `Intensity` is disabled as well as the item
    `Recoil lines` within the menu item `Information` in the new plot
    window
6.  Check that the `File` menu only has one menu item, `Close`
7.  Change the rectangle by changing its size or dragging it to a
    different area of the slice plot. The cut plot should update
    accordingly.
8.  Click on `Save Cut to Workspace` and check the `MD Histo` tab of the
    Workspace Manager to verify that the new workspace was added
9.  Click on Flip Integration Axis. The x axis label changes from
    `Energy Transfer (meV)` to $|Q| (\mathrm{\AA}^{-1})$ or vice versa,
    depending on the initial label.

<figure>
<img src="../../../../docs/source/images/flip_integration_axis.png"
class="align-center" style="width:10.0%"
alt="../../../../docs/source/images/flip_integration_axis.png" />
</figure>

<figure>
<img src="../../../../docs/source/images/interactive_cuts.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/interactive_cuts.png" />
</figure>

### 5. Overplot Bragg Peaks

1.  Navigate to the `Information` menu on the cut plot
2.  Select `Aluminium` from the submenu for `Bragg peaks`. Green lines
    should appear on the cut plot with a respective legend entry.
3.  Deselect `Aluminium` form the submenu for `Bragg peaks`. Both green
    lines and the respective legend entry should disappear.

<figure>
<img src="../../../../docs/source/images/cut_with_bragg_peaks.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/cut_with_bragg_peaks.png" />
</figure>

### 6. Generate a Script

1.  Navigate to the `Cut` tab
2.  In the row labelled `along`, select `|Q|` and set the `from` value
    to `0` and the `to` value to `10`
3.  In the row labelled `over`, set the `from` value to `-5` and the
    `to` value to `5`
4.  Click `Plot`. A new window with a cut plot should open.
5.  Navigate to the `Information` menu on the cut plot
6.  Select `Aluminium` from the submenu for `Bragg peaks`. Green lines
    should appear on the cut plot with a respective legend entry.
7.  Navigate to the `File` menu on a cut plot. Please note that this
    needs to be a cut plot created via the `Cut` tab and not an
    interactive cut.
8.  Select `Generate Script to Clipboard` and paste the script into the
    Mantid editor. Please note that on Linux `Ctrl + V` might not work
    as expected. Use `shift insert` instead in this case.
9.  Run the script and check that the same cut plot is displayed

## The Command Line Interface

### 1. Use the Mantid Editor

1.  Close all plots currently open but not the MSlice interface
2.  Copy the following code into the Mantid editor. You might have to
    modify the file path for the Load command to the correct location of
    `MAR21335_Ei60meV.nxs`.

``` python
import mslice.cli as mc

ws = mc.Load('C:\MAR21335_Ei60meV.nxs')
wsq = mc.Cut(ws, '|Q|', 'DeltaE, -1, 1')
mc.PlotCut(wsq)

ws2d = mc.Slice(ws, '|Q|, 0, 10, 0.01', 'DeltaE, -5, 55, 0.5')
mc.PlotSlice(ws2d)
```

### 2. Run an Example Script

1.  Run the script.
2.  There should be two new windows with a slice plot and a cut plot

<figure>
<img src="../../../../docs/source/images/output_mslice_script.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/output_mslice_script.png" />
</figure>

### 3. Use the Jupyter QtConsole

1.  Repeat the same test by copying the script into the Jupyter
    QtConsole of the MSlice interface

<figure>
<img src="../../../../docs/source/images/mslice_jupyter_qtconsole.png"
class="align-center" style="width:80.0%"
alt="../../../../docs/source/images/mslice_jupyter_qtconsole.png" />
</figure>

### 4. Run Another Example Script in the Mantid Editor

1.  Select the `MAR21335_Ei60meV` workspace in the `Workspace Manager`,
    click `Compose` and then `Scale`
2.  Enter a scale factor of 1.0 and click `Ok`
3.  Select the `MAR21335_Ei60meV` workspace again and click `Subtract`
4.  Select the `MAR21335_Ei60meV_scaled` workspace and leave the
    self-shielding factor as 1.0, then click `Ok`
5.  Select the `MAR21335_Ei60meV_subtracted` workspace and click
    `Display` in the `Slice` tab
6.  Verify that all values are zeros
7.  Navigate to the `File` menu on the slice plot, select
    `Generate Script to Clipboard` and paste the script into the Mantid
    editor
8.  Close the slice plot with all zeros
9.  Run the script in the Mantid editor and verify that a slice plot
    with all zeros is reproduced
