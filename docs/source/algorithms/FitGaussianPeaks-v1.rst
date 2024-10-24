.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
The algorithm takes:

- a table with a single column where each row contains the centre of the peak

- a workspace containing the data to be fitted in the first spectra and a background for the data.
  This is important to produce sensible results from the Poisson weight.

It then performs the following steps:

1.  Perform a preliminary fit of each peak separately to obtain an estimate for the peak parameters.

2.  Fit all the peaks together using a gaussian for each peak.

3.  Create a table where every row contains the parameters and error for a peak.

4.  For some peaks the fit might produce unreasonably large errors (above :math:`10^7`). These peaks will not be
    included in the table.
    Instead they will be refitted with tighter constraints that will return sensible values for the parameters
    most of the times.
    These parameters will be inserted in a second table structured as the first.

5.  The fit will be evaluated using unweighted :math:`\chi^2` and a poisson cost function (see below).
    For the Poisson fit, the background is added.
    The result of the two are included in a third table.

Cost functions
--------------

-  :math:`\chi^2`:
    The result of the fit is compared with the data using the equation:
    :math:`{1 \over N} \sum_{i=1}^{N} {(m_i - d_i) \over \sigma_i}^2`

    Where :math:`m_i` is the i-th data point of the result of the fit, :math:`d_i` is the i-th data point
    of the data to be fitted and :math:`\sigma_i` is the error on the i-th data point.

-  Poisson:
    The result of the fit and input data are filtered to remove zeros in the fitted data.
    They are then compared using the equation:
    :math:`\sum_{i=1}^{N} (-m_i + d_i \ln(m_i))`

    Where :math:`m_i` is the i-th data point of the result of the fit and :math:`d_i` is the i-th
    data point of the data to be fitted.
    This is the natural logarithm of the cost, calculated as: :math:`\prod_{i=1}^{N} \exp(-m_i + d_i \ln(m_i))`

**Example - Finding two simple gaussian peaks.**

.. testcode:: SimpleGaussianFit

    # Function for a gaussian peak
    def gaussian(xvals, centre, height, sigma):
        exp_val = (xvals - centre) / (np.sqrt(2) * sigma)

        return height * np.exp(-exp_val * exp_val)


    # Creating two peaks
    x_values = np.linspace(0, 100, 1001)
    centre = [25, 75]
    height = [35, 20]
    width = [10, 5]
    y_values = gaussian(x_values, centre[0], height[0], width[0])
    y_values += gaussian(x_values, centre[1], height[1], width[1])
    background = 10 * np.ones(len(x_values))

    # Generating a table with a guess of the position of the centre of the peaks
    peak_table = CreateEmptyTableWorkspace()
    peak_table.addColumn(type='float', name='Approximated Centre')
    peak_table.addRow([centre[0]+2])
    peak_table.addRow([centre[1]-3])

    # Generating a workspace with the data and a flat background
    data_ws = CreateWorkspace(DataX=np.concatenate((x_values, x_values)),
                              DataY=np.concatenate((y_values, background)),
                              DataE=np.sqrt(np.concatenate((y_values, background))),
                              NSpec=2)

    # Fitting the data
    parameters, refitted_parameters, cost = FitGaussianPeaks(
        InputWorkspace=data_ws,
        PeakGuessTable=peak_table,
        CentreTolerance=3.0,
        EstimatedPeakSigma=5,
        MinPeakSigma=0.0,
        MaxPeakSigma=30.0,
        GeneralFitTolerance=0.1,
        RefitTolerance=0.001
    )

    peak1 = parameters.row(0)
    peak2 = parameters.row(1)
    print('Peak 1: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.1f}'
          .format(peak1['centre'], peak1['error centre'],
                  peak1['height'], peak1['error height'],
                  peak1['sigma'], peak1['error sigma']))
    print('Peak 2: centre={:.2f}+/-{:.2f}, height={:.2f}+/-{:.2f}, sigma={:.2f}+/-{:.1f}'
          .format(peak2['centre'], peak2['error centre'],
                  peak2['height'], peak2['error height'],
                  peak2['sigma'], peak2['error sigma']))
    print('Chi2 cost: {:.3f}'.format(cost.column(0)[0]))
    print('Poisson cost: {:.3f}'.format(cost.column(1)[0]))

Output (the number on your machine may differ slightly from these:

.. testoutput:: SimpleGaussianFit

    Peak 1: centre=25.00+/-0.11, height=35.00+/-0.47, sigma=10.00+/-0.1
    Peak 2: centre=75.00+/-0.10, height=20.00+/-0.49, sigma=5.00+/-0.1
    Chi2 cost: 0.000
    Poisson cost: 46444.723

.. categories::
.. sourcelink::
