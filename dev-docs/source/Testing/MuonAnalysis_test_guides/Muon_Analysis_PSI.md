# Muon Unscripted Testing: PSI

```{contents}
:local:
```

## Introduction

These are unscripted tests for PSI data and introduce background
corrections. For continous sources background corrections are very
important. Fitting a flat background to the data (sometimes along with a
exp decay), it is possible to remove the background from the data.

------------------------------------------------------------------------

## Loading Data Test

- Open Muon Analysis
- On the **Home** tab set the instrument to **PSI**
- The load current run button should be greyed out
- Load **deltat_tdc_dolly_1529.bin** from the unit test data using the
  **Browse** button
- Set <span class="title-ref">Rebin</span> to
  <span class="title-ref">Fixed</span> and enter a value of
  <span class="title-ref">5</span>
- You will get 4 lines that all curve upwards
- If you tick the <span class="title-ref">Plot raw</span> option the
  data will change

------------------------------------------------------------------------

## Auto Background Corrections Test

- Go to the **Corrections** tab

- For the `Background` select `Auto`

- The plots will now average around 0

-

  The values should be (approximatley):

  :   - Forw: `19.2`
      - Back: `93.6`
      - Left: `104.4`
      - Rite: `77.2`

- Tick the `use Raw` box and the values will change

- The new values should be similar (within 1) to before

------------------------------------------------------------------------

## Flat Background Corrections Test

- Change the `Select Function` to `Flat Background`
- The curves will be curved upwards again and the table will report an
  error
- The **Corrections** tab will show a red asterisk
- Go to the **Home** tab
- Untick `Time Zero`
- Switch the plot from `Asymmetry` to `Counts`
- Set the `Time Zero` to be `0`
- The plots will now show a large peak, prior to this is only background
- Using the options at the bottom of the plot, set the x range from
  `0.0` to `1.0`
- Go to the **Corrections** tab
- Change the `Start X` to `0.0`, it will automatically change to `0.006`
- Change the `End X` to just before the peak in the plot
- This will fit a flat background to the data before the peak
- The results should be similar to before

------------------------------------------------------------------------

## Manual Background Corrections Test

- Change the plot back to `Asymmetry`
- Change the `Background` to `Manual`
- Untick the `Apply parameter change to all domains`
- Change some of the background values, but leave some as they are
- Go to the **Home** tab
- Tick the `Time Zero` box
- The plots that you changed will drift away from zero at large times
- The plots that you did not change will average around zero
