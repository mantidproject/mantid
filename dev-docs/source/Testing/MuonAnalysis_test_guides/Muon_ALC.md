# Muon Unscripted Testing: ALC

::: {.contents local=""}
Table of Contents
:::

## Introduction

These tests looks at the ALC interface. This interface loads a series of
runs and automatically calculate the asymmetry. The asymmetry
calculation is determined by the settings on the load page and are for a
pair. It is often used to find peaks in the asymmetry as a function of a
sample log (e.g. temperature).

------------------------------------------------------------------------

## ALC peak fitting

**Time required 10 - 15 minutes**

- Open **ALC** (*Interfaces* \> *Muon* \> *ALC*)
- Change *Instrument* to **EMU**
- In the loading bar enter `81061-142`
- The text undearneath will change colour while it searches for the data
  (orange)
- Once it has found the data the text will turn green
- Select `field_danfysik` for the sample log
- Set the calculation max to `16`
- Press the `Load` button, this can be slow
- Once it is complete a plot will appear showing one dominant peak
- Press the `Baseline modelling` button at the bottom of the interface
- Add a `LinearBackground` fitting function
- Add a section, drag the upper bound to be just before the start of the
  main peak
- Add a second section, type into the `Start X` box to place it after
  the main peak
- Press fit
- Click the `Corrected data` tab and the baseline should be about zero
- Press the `Peak fitting` button at the bottom of the interface
- Add a `Gaussian` fitting function, set the parameters to give a decent
  guess (use plot guess to check)
- Press fit, you should then have a nice fit to the data

------------------------------------------------------------------------

## ALC MultiPeriod Data

**Time required 5 minutes**

- In the loading bar enter `81070-81095,81120-40`
- Select `field_danfysik` for the sample log
- Press the `Period info` button and pop up will appear with 2 rows
- Untick `subtract` in the periods section
- Set the calculation max to `5`
- Press the `Load` button, this can be slow
- The new data will look a mess
- Press the `External Plot` button at the bottom, a new plot will appear

------------------------------------------------------------------------

## ALC Single Period Data

**Time required 5 minutes**

- Change *Instrument* to **HIFI**
- In the loading bar enter `134028-39`
- Select `run_number` for the sample log
- Press the `Period info` button and pop up will appear with 1 row
- Change the dead time correction to be `From Data File`
- The `subtract` checkbox should be disabled
- Set the calculation max to `15`
- Press the `Load` button, this can be slow
- The new data will look like a decay, with a max value just below `0.2`
- In the `Grouping` section change the `Alpha` to be `1.3`
- Press the `Load` button
- The new data will look like a decay, with a max value just below
  `0.07`
- In the `Grouping` section change it to `Custom`
- For `Forward` and `Backward` give some valid detectors (e.g. 1, 64)
- Press Load and the data will change
- Check that invalid detector values (i.e. letters, decimals) result in
  a warning when pressing `Load`
