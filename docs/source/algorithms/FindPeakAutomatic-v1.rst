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

    # Creating two peaks on an exponential background with gaussian noise
    x_values = np.linspace(0, 100, 1001)
    centre = [25, 75]
    height = [35, 20]
    width = [3, 2]
    y_values = gaussian(x_values, centre[0], height[0], width[0])
    y_values += gaussian(x_values, centre[1], height[1], width[1])
    y_values += np.abs(40*np.exp(-0.05*x_values) + np.random.randn(len(x_values)))
    data_ws = CreateWorkspace(DataX=x_values, DataY=y_values, DataE=np.sqrt(y_values))

    # Fitting the data
    FindPeakAutomatic(InputWorkspace=data_ws,
                      AcceptanceThreshold=0.8,
                      SmoothWindow=70,
                      PlotPeaks=False,
                      PeakPropertiesTableName='properties',
                      RefitPeakPropertiesTableName='refit_properties')
    peak_properties = mtd['properties']
    refitted_peak_properties = mtd['refit_properties']

    peak1 = peak_properties.row(0)
    peak2 = peak_properties.row(1)
    print('Peak 1: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak1['centre'], peak1['error centre'],
                  peak1['height'], peak1['error height'],
                  peak1['sigma'], peak1['error sigma']))
    print('Peak 2: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.2f}'
          .format(peak2['centre'], peak2['error centre'],
                  peak2['height'], peak2['error height'],
                  peak2['sigma'], peak2['error sigma']))

Output:

.. testoutput:: GaussianPeaksExponentialBackground

    Peak 1: centre=25.25+/-0.09, height=34.44+/-1.05, sigma=3.03+/-0.08
    Peak 2: centre=74.97+/-0.07, height=20.12+/-0.77, sigma=2.20+/-0.05


.. categories::
.. sourcelink::