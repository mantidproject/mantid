# Inelastic Corrections Testing

::: {.contents local=""}
:::

*Prerequisites*

- Download the [ISIS Sample Data](http://download.mantidproject.org)

## Container Subtraction tab

**Time required 1 - 3 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Corrections`
2.  Go to the `Container Subtraction` tab
3.  With the Sample combo box set to `File` click browse and select the
    file `irs26176_graphite002_red` from the Sample Data folder
4.  With the Container combo box set to `File` click browse and select
    the file `irs26174_graphite002_red` from the Sample Data folder
5.  Click `Run`
6.  This should plot a blue subtracted line on the embedded plot. A
    workspace ending in `_red` should be produced
7.  Change the `Spectrum` value and the plot should be updated with the
    corresponding workspace index
8.  Check that when clicking `Plot Current Preview`, it will open a
    pop-up window with the current preview plot
9.  In the `Output Options`, select `Open Slice Viewer` using the down
    arrow, it should open the Slice Viewer with the generated dataset
10. In the `Output Options`, select `Plot 3D Surface` using the down
    arrow, it should open a surface plot for the generated dataset
11. In the `Output Options`, select indices `50-51`. A red asterisk
    should appear, preventing you from using `Plot Spectra`
12. In the `Output Options`, select indices `49-50`. It should be
    possible to click `Plot Spectra` to plot a two spectra graph

## Calculate Monte Carlo Absorption Tab

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Corrections`
2.  Go to the `Calculate Monte Carlo Absorption` tab
3.  With the Input Workspace set to `File` click browse and select the
    file `irs26176_graphite002_red` from the Sample Data folder
4.  Choose the Sample Shape to be `Flat Plate`
5.  Sample Height & Width should be `1.0`
6.  Sample Thickness, Container Front Thickness and Container Back
    Thickness should be `0.1`
7.  Sample mass density should be `1.0`. The Sample formula should be
    `H2-O`
8.  Container mass density should be `6.0`. The Container formula should
    be `V`
9.  Click `Run` and wait.
10. A group workspace ending in `_Corrections` should be created. Keep
    this workspace for the Apply Absorption Corrections test.
11. In the `Output Options`, the combo box should have four entries
    ending in `_ass`, `_assc`, `_acsc` and `_acc`.
12. In the `Output Options`, try using the `Plot Wavelength` and
    `Plot Angle` options.

## Apply Absorption Corrections Tab

**Time required 1 - 3 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Corrections`.
2.  Go to the `Apply Absorption Corrections` tab.
3.  With the Sample combo box set to `File` click browse and select the
    file `irs26176_graphite002_red` from the Sample Data folder.
4.  With the Corrections combo box set to `Workspace`, and select the
    workspace ending in `_Corrections` from the previous test.
5.  Tick `Use Container` and select the file `irs26174_graphite002_red`
    from the Sample Data folder.
6.  Click `Run` and wait.
7.  This should plot a blue corrected line on the embedded plot. A
    workspace ending in `_red` should be produced
8.  In the `Output Options`, select `Open Slice Viewer` using the down
    arrow, it should open the Slice Viewer with the generated dataset
9.  In the `Output Options`, select `Plot 3D Surface` using the down
    arrow, it should open a surface plot for the generated dataset
10. In the `Output Options`, select indices `50-51`. A red asterisk
    should appear, preventing you from using `Plot Spectra`
11. In the `Output Options`, select indices `49-50`. It should be
    possible to click `Plot Spectra` to plot a two spectra graph
