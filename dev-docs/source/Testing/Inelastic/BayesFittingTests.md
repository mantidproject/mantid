# Inelastic Bayes Fitting Testing

::: {.contents local=""}
:::

*Prerequisites*

- Download the [ISIS Usage Data](http://download.mantidproject.org)

## ResNorm tab

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Bayes fitting`
2.  Go to the `ResNorm` tab
3.  With the vanadium combo box set to `File` click browse and select
    the file `irs26173_graphite002_red` from the Usage Data folder
4.  With the resolution combo box set to `File` click browse and select
    the file `irs26173_graphite002_res` from the Usage Data folder
5.  Click `Run`
6.  This should produce two workspaces with `ResNorm` and `ResNorm_Fit`
    suffixes
7.  Make sure that moving the black sliders in the plot will change the
    value of `EMin` and `EMax`
8.  Change the `Preview Spectrum` value and the plot should be updated
    with the corresponding workspace index
9.  Check that the maximum for `Preview Spectrum` is 9 (the last
    workspace index in the vanadium workspace)
10. Check that when clicking `Plot Current Preview`, it will open a
    pop-up window with the current preview plot
11. In the `Output Options`, select `All` and click `Plot`, it should
    open two plots suffixed with `Intensity` and `Stretch`
12. Plot individual results by selecting other options, it should only
    open one plot with the selected result

## Quasi Tab

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Bayes fitting`
2.  Go to the `Quasi` tab
3.  Click on `Manage Directories` and set the default save directory to
    any location in your machine
4.  With the sample combo box set to `File` click browse and select the
    file `irs26176_graphite002_red` from the Usage Data folder
5.  With the resolution combo box set to `File` click browse and select
    the file `irs26173_graphite002_res` from the Usage Data folder
6.  Click `Run`
7.  This should produce three workspaces with `Fit`, `Prob` and `Result`
    suffixes
8.  Make sure that moving the black sliders in the plot will change the
    value of `EMin` and `EMax`
9.  Run the tab with different options in the `Fit Options` section
10. Check that the maximum for `Preview Spectrum` is 9 (the last
    workspace index in the sample workspace)
11. Check that when clicking `Plot Current Preview`, it will open a
    pop-up window with the current preview plot
12. In the `Output Options`, select `All` and click `Plot`, it should
    open three plots with `Prob`, `Amplitude` and `FWHM` data. The
    `Amplitude` and `FWHM` plots will have a title ending in `_Result`.
13. Plot individual results by selecting other options, it should only
    open one plot with the selected attribute

## Stretch Tab

**Time required 3 - 5 minutes**

------------------------------------------------------------------------

1.  Go to `Interfaces` \> `Inelastic` \> `Bayes fitting`
2.  Go to the `Stretch` tab
3.  Click on `Manage Directories` and set the default save directory to
    any location in your machine
4.  With the sample combo box set to `File` click browse and select the
    file `irs26176_graphite002_red` from the Usage Data folder
5.  With the resolution combo box set to `File` click browse and select
    the file `irs26173_graphite002_res` from the Usage Data folder
6.  Click `Run`
7.  This should produce two workspaces with `Stretch_Contour` and
    `Stretch_Fit` suffixes
8.  Make sure that moving the black sliders in the plot will change the
    value of `EMin` and `EMax`
9.  Run the tab with different settings in the `Fit Options` section
10. Check that the maximum for `Preview Spectrum` is 9 (the last
    workspace index in the sample workspace)
11. Check that when clicking `Plot Current Preview`, it will open a
    pop-up window with the current preview plot
12. In the `Output Options`, select `All` and click `Plot`, it should
    open two plots with `Sigma` and `Beta` data
13. Plot individual results by selecting other options, it should only
    open one plot with the selected attribute
14. Click `Plot Contour` to plot the stretch contour workspace
