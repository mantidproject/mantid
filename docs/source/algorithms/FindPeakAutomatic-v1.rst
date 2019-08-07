.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm takes a workspace containing a spectrum and will attempt to find the number and parameters of
all the peaks (assumed to be gaussian).

To do so the algorithm performs the following:

1. The data is loaded, invalid values discarded and the run is cropped to the user defined window.

2. If no error value is provided the error is calculated as the square root of the y values.

3. The morphological operations of erosion and dilation are used to calculate a background which
   is then removed.

4. All points higher than the two neighbours are identified as potential peaks.

5. The list of possible peaks is identified and sorted by "prominence".

6. The null hypothesis (i.e. no peaks present) is tested and its cost calculated.

7. The best peak not yet considered from #4 is added to the list of fitted peaks and the cost calculated.

8. If the cost after adding the peak improves on the cost without the peak by more than the `AcceptanceThreshold`
   threshold then the peak is added to the list of true peaks. Otherwise the peak is considered bad and discarded.

9. If more than `BadPeaksToConsider` have been encountered since the last good peak then the program terminates
   and the list of peaks that best fit the data is returned. Otherwise the procedure is repeated from step #6.

To fit a list of peaks and calculate the cost algorithm :ref:`FitGaussianPeaks <algm-FitGaussianPeaks>`
is used.
If the option `PlotPeaks` is selected then after completing the algorithm a plot of the input data is generated
and the peaks marked with red `x`.

Improving the fit
-----------------

The result of the fit is strongly dependent on the parameter chosen. Unfortunately these depend on the particular run
one wishes to fit. Therefore, while the default choice will often generate reasonable answers it is often necessary to
adjust some of them.

The most important to this effect are:

- `AcceptanceThreshold`, the value of this depends on the cost function used.
  For chi2, reasonable values range between 0.0001-0.1. For poisson cost, reasonable value range between 1-100.
  In both cases, higher values correspond to stronger constraints, and will lead to less peaks found.

- `SmoothWindow`, this affects the background-finding. Low values of this parameter will only leave the narrowest peaks,
  eliminating noise most effectively but also potentially getting rid of peaks of interest. Conversely, higher values
  will be less aggressive, leaving broader peaks and noise features to be fitted.

- `BadPeaksToConsider`, if this parameter is too low, there is a possibility that valid peaks will be discarded because
  some noise features were previously encountered. However, higher values will strongly affect the performance and
  will tend to introduce unwanted noise in the final peak count.

- `EstimatePeakSigma`, this is an initial guess for the standard deviation of the peaks in the data. It becomes
  especially important when trying to differentiate closely spaced features, where lower values of this parameter will
  usually perform better that higher ones.

**Example - Find two gaussian peaks on an exponential background with added noise**

.. testcode:: GaussianPeaksExponentialBackground

    # Function for a gaussian peak
    def gaussian(xvals, centre, height, sigma):
        exp_val = (xvals - centre) / (np.sqrt(2) * sigma)

        return height * np.exp(-exp_val * exp_val)


    np.random.seed(1234)

    # Creating two peaks on an exponential background with gaussian noise
    x_values = np.linspace(0, 1000, 1001)
    centre = [250, 750]
    height = [350, 200]
    width = [3, 2]
    y_values = gaussian(x_values, centre[0], height[0], width[0])
    y_values += gaussian(x_values, centre[1], height[1], width[1])
    y_values_low_noise = y_values + np.abs(400 * np.exp(-0.005 * x_values)) + 30 + 0.1*np.random.randn(len(x_values))
    y_values_high_noise = y_values + np.abs(400 * np.exp(-0.005 * x_values)) + 30 + 10*np.random.randn(len(x_values))
    low_noise_ws = CreateWorkspace(DataX=x_values, DataY=y_values_low_noise, DataE=np.sqrt(y_values_low_noise))
    high_noise_ws = CreateWorkspace(DataX=x_values, DataY=y_values_high_noise, DataE=np.sqrt(y_values_high_noise))

    # Fitting the data with low noise
    FindPeakAutomatic(InputWorkspace=low_noise_ws,
                      AcceptanceThreshold=0.2,
                      SmoothWindow=30,
                      EstimatePeakSigma=2,
                      MaxPeakSigma=5,
                      PlotPeaks=False,
                      PeakPropertiesTableName='properties',
                      RefitPeakPropertiesTableName='refit_properties')
    peak_properties = mtd['properties']
    peak_low1 = peak_properties.row(0)
    peak_low2 = peak_properties.row(1)

    # Fitting the data with strong noise
    FindPeakAutomatic(InputWorkspace=high_noise_ws,
                      AcceptanceThreshold=0.2,
                      SmoothWindow=30,
                      EstimatePeakSigma=2,
                      MaxPeakSigma=5,
                      PlotPeaks=False,
                      PeakPropertiesTableName='properties',
                      RefitPeakPropertiesTableName='refit_properties')
    peak_properties = mtd['properties']
    peak_high1 = peak_properties.row(0)
    peak_high2 = peak_properties.row(1)

    print('Low noise')
    print('Peak 1: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak_low1['centre'], peak_low1['error centre'],
                  peak_low1['height'], peak_low1['error height'],
                  peak_low1['sigma'], peak_low1['error sigma']))
    print('Peak 2: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak_low2['centre'], peak_low2['error centre'],
                  peak_low2['height'], peak_low2['error height'],
                  peak_low2['sigma'], peak_low2['error sigma']))
    print('')

    print('Strong noise')
    print('Peak 1: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak_high1['centre'], peak_high1['error centre'],
                  peak_high1['height'], peak_high1['error height'],
                  peak_high1['sigma'], peak_high1['error sigma']))
    print('Peak 2: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak_high2['centre'], peak_high2['error centre'],
                  peak_high2['height'], peak_high2['error height'],
                  peak_high2['sigma'], peak_high2['error sigma']))

Output:

.. testoutput:: GaussianPeaksExponentialBackground

    Low noise
    Peak 1: centre=250.01+/-0.09, height=352.47+/-10.91, sigma=3.06+/-0.08
    Peak 2: centre=750.00+/-0.09, height=200.03+/-9.13, sigma=2.01+/-0.07

    Strong noise
    Peak 1: centre=250.00+/-0.09, height=360.33+/-10.83, sigma=3.11+/-0.08
    Peak 2: centre=749.90+/-0.08, height=194.86+/-7.82, sigma=2.47+/-0.07

.. categories::
.. sourcelink::