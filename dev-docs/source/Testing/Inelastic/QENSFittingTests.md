# Inelastic QENS Fitting Testing

::: {.contents local=""}
:::

*Prerequisites*

- Download the [ISIS Sample Data](http://download.mantidproject.org)
- Make sure that the data set is on your list of search directories

## MSD tab

*Preparation*

- The `_eq.nxs` file from the
  `Elwin Indirect Inelastic <elwin_inelastic_test>` test

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `QENS fitting`
2.  Go to the `MSD` tab
3.  Click `Add Workspace`
4.  With the combo box set to `File` click browse and select the file
    that you saved in the previous test
5.  Check `All Spectra`
6.  Click `Add` and close the dialogue.
7.  Set `Fit type` to Gaussian
8.  Click `Run`
9.  This should produce a plot of the fitted function in the interface
10. Change `End X` to 1.0
11. Click `Run`
12. Repeat the previous steps with `Peters` and `Yi` functions
13. Try run fits using the different `Minimizer` options (except
    FABADA), each time change the `End X` value either + or - 0.1

## I(Q, T) tab

*Preparation*

- The `_iqt` workspace from the
  `I(Q, T) Indirect Inelastic <iqt_inelastic_test>` test

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `QENS fitting`
2.  Go to the `I(Q, T)` tab
3.  Click `Add Workspace`
4.  With the combo box set to `Workspace` select the `_iqt` workspace
    from the previous test
5.  Check `All Spectra`
6.  Click `Add` and close the dialogue.
7.  Set `Exponential` to 1
8.  In the data table set `EndX` for WS Index 0 to 0.14
9.  Using shift select the `EndX` for all spectra and click unify range,
    this should set the `EndX` for all spectra to 0.14
10. Click `Run`
11. This should produce a fit and a difference plot in the window
12. Click `Plot current preview` this should open a plot with three
    datasets plotted
13. Select the runs 6 - 50 and click `Remove`
14. Click `Run`
15. Select Lifetime from the `Output` drop-down
16. Click `Plot` this should open a new plot with the lifetimes plotted

## Convolution tab

*Preparation*

- ISIS Sample data set, [available
  here](http://download.mantidproject.org/)

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `QENS fitting`
2.  Go to the `Convolution` tab
3.  Click `Add Workspace`
4.  With the combo box's set to `File`
5.  Click browse and load the `irs26176_graphite002_red.nxs` file from
    the sample data
6.  Click browse and load the resolution file
    `irs26173_graphite002_res.nxs` from the sample data
7.  In Workspace Indices enter `0-5`
8.  Click `Add` and close the dialogue.
9.  Set `Lorentzians` to 2
10. Set `Max iterations` to 400
11. Click `Run`
12. Three new workspaces should be created in the main GUI -
    `Parameters`, `Result` and `Workspaces`
13. In the `Mini plots` widget, change `Plot Spectrum` to 3
14. Click `Fit Single Spectrum` the plot should update and new
    workspaces are created in the main Mantid GUI
15. Remove spectra 0-2, then remove spectra 5. do these as separate
    removals.
16. Click `Run`; the plot should update and new workspaces are created
    in the main Mantid GUI
17. Try the various `Plot` options in the interface
    1)  `Output` drop-down set to All and click `Plot` - should give 5
        separate plots per input spectra
    2)  `Plot Current Preview` - should result in a plot with three
        datasets
    3)  Enable the `Plot Guess` checkbox - should not change anything,
        but should not break anything either!
18. Change the `Fit type` to different functions and run fits

## Function (Q) tab

*Preparation*

- The `_Result` workspace output from the
  `Convolution tab <convolution_inelastic_test>` test

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `QENS fitting`
2.  Go to the `Function (Q)` tab
3.  Click `Add Workspace`
4.  With the combo box set to `Workspace` select the `0-5__Result`
    workspace from the previous test
5.  In Parameter Name select `f1.f0.FWHM`
6.  Click `Add` and close the dialogue.
7.  Under `Fit Type` select `TeixeiraWater`
8.  Click `Run`
9.  Three new workspaces should be created in the main GUI -
    `Parameters`, `Result` and `Workspaces`
10. Change the `Fit type` to different functions and run fits
