
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates a measure of the goodness of a fit in four ways.

The ChiSquared property returns the sum of squares of differences between the calculated and measured values:

:math:`\chi_{1}^{2} = \sum_{i} (y_i - f_i)^2`

where :math:`y_i` and :math:`f_i` are the measured and calculated values at i-th point.

The ChiSquaredDividedByDOF is ChiSquared divided by the number of degrees of freedom (DOF):

:math:`\chi_{2}^{2} = \frac{1}{DOF}\sum_{i} (y_i - f_i)^2`

:math:`DOF = N_d - N_p` where :math:`N_d` is the number of data points used in fitting and :math:`N_p`
is the number of free (not fixed or tied) parameters of the function.

The ChiSquaredWeighted property sums the squares of the differences divided by the data errors:

:math:`\chi_{3}^{2} = \sum_{i} \left(\frac{y_i - f_i}{\sigma_i}\right)^2`

Finally, ChiSquaredWeightedDividedByDOF is

:math:`\chi_{4}^{2} = \chi_{3}^{2} / DOF`

Parameter errors
################

Setting the Output property to a non-empty string makes the algorithm explore the surface of the :math:`\chi^{2}`
around its minimum and estimate the standard deviations for the parameters. The value of the property is a base name
for two output table workspaces: '<Output>_errors' and '<Output>_pdf'. The former workspace contains parameter error
estimates and the latter shows :math:`\chi^{2}`'s 1d slices along each parameter (keeping all other fixed).


The procedure for calculating errors of parameters is described in Chapter 15 of Numerical recipes in C [1] and Chapter 9
of Statistical Data Analysis [2]. Here, we summarise the main results.

Consider the input dataset :math:`D_0`, with fit parameters :math:`\mathbf a_0`. Assuming Gaussian noise in the data, it is possible
to create :math:`n` artificial datasets :math:`D_j` for :math:`j=1,2,..,n`. If we were to run the fit on each dataset,
a set of parameters :math:`\mathbf a_j` would be obtained. The distribution of these parameters,
about our original parameter set :math:`\delta \mathbf a =  \mathbf a_j - \mathbf a_0` is described by the multivariate normal distribution,

.. math::
    P(\delta \mathbf a ) \propto exp (-\Delta \chi^2)

where :math:`\Delta \chi^2=\chi^2(\mathbf a_J) - \chi^2(\mathbf a_0)`, is the difference between the chi squared statistics obtained for each parameter set,

.. math::
    \Delta \chi^2 = \sum_{i \in D_0} \left ( \frac{y_i -f(x_i;\mathbf a_J)}{\sigma_i}\right)^2 - \left ( \frac{y_i -f(x_i;\mathbf a_0)}{\sigma_i}\right)^2

If we consider the variation of a single parameter :math:`a_k`, while the other parameters are the values that minimize :math:`\chi^2`,
the quantity :math:`\Delta \chi^2` will be distributed as a chi squared distributed, :math:`f_{\chi^2}(x; \nu)` with 1 degree of freedom,
:math:`\nu=1`. From this distribution, the probability of finding increases less than :math:`\Delta \chi^2` can be found from the integral,

.. math::
	p = P(X < \Delta \chi^2 ) = \int_0^{\Delta \chi^2} f_{\chi^2}(x; 1) dx

where :math:`p` is the desired confidence level. For instance, if we consider a confidence level of :math:`p=68.3\%` (1-sigma),
we find a critical value for the chi squared fluctuation of :math:`\Delta \chi^2 < 1`. From this desired confidence level,
we can then calculate the parameter variation, (:math:`\sigma_{left}`, :math:`\sigma_{right}`), that increases chi squared by 1
and obtain a :math:`68.3\%` confidence interval for the single parameter :math:`a_k`:

.. math::
    P( a_k - \sigma_{left} < a_k <  a_k + \sigma_{right}) = 68.3\%

This algorithm obtains the 1-sigma confidence level by varying a single parameter at a time and recording the extremums
of the values that increase :math:`\chi^2` by :math:`1`. The results are outputted in the table '<Output>_errors',
which reports the left and right 1-sigma errors. In this table, an additional value is reported, termed the quadratic error.
This is the 1-sigma error obtained from a Taylor expansion of the chi squared function about its minimum:

.. math::
	\chi^2(\mathbf a + \delta \mathbf a) = \chi^2(\mathbf a)_{min} + \delta \mathbf a^T  \mathbf C \delta \mathbf a

where :math:`\mathbf{C}` is the covariance matrix obtained from least squares fitting of the data, see the :ref:`Fitting <algm-Fit>` documentation for further details.
For a linear model (or approximately linear model) the quadratic error should be equal to the left and right errors, i.e it will be symmetric.
This can be seen in the following example

.. code-block:: python

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    # create the data
    x = np.linspace(1, 10, 250)
    np.random.seed(0)
    y = 1 + 2.0*x + 0.4*np.random.randn(x.size)
    ws = CreateWorkspace(x, y)

    # run the fit
    func = "name=LinearBackground, A0=1, A1=2";
    func_ = FunctionFactory.Instance().createInitialized(func)

    fit_output = Fit(InputWorkspace=ws, Function=func_, Output="LinearFit")
    a0_fit = fit_output.OutputParameters.column(1)[0]
    a1_fit = fit_output.OutputParameters.column(1)[1]

    # explore the chi squared profile for the fit parameters
    fitted_func = "name=LinearBackground, A0={}, A1={}".format(a0_fit, a1_fit);
    out = CalculateChiSquared(fitted_func, ws, Output="LinearProfile", weighted=True)


    # print left and right errors of parameters
    # you should note that they are approx equal to the quadratic error for this linear model
    error_table = mtd["LinearProfile_errors"]
    lerror_a0 = error_table.column(3)[0]
    rerror_a0= error_table.column(4)[0]
    qerror_a0 = error_table.column(5)[0]

    print("Error bounds of A0 are {} and {}, with quadratic estimate {}".format(lerror_a0, rerror_a0, qerror_a0))
    lerror_a1 = error_table.column(3)[1]
    rerror_a1= error_table.column(4)[1]
    qerror_a1 = error_table.column(5)[1]
    print("Error bounds of A1 are {} and {}, with quadratic estimate {}".format(lerror_a1, rerror_a1, qerror_a1))


For a non-linear model, it's possible that the left and right variances will not be equal, leading to an asymmetric error.
This is shown in the example below:

.. code-block:: python

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np


    # create decaying exponential data
    x = np.linspace(1, 10, 250)
    np.random.seed(0)
    y = 3.0*np.exp(-x/2) + 0.1*np.random.randn(x.size)
    ws = CreateWorkspace(x, y)

    # run the fit
    func = "name=ExpDecay,Height=3.0, Lifetime=0.5";
    func_ = FunctionFactory.Instance().createInitialized(func)

    fit_output = Fit(InputWorkspace=ws, Function=func_, Output="ExpFit")
    height_fit = fit_output.OutputParameters.column(1)[0]
    lifetime_fit = fit_output.OutputParameters.column(1)[1]

    # explore the chi squared profile for the fit parameters
    fitted_func = "name=ExpDecay, Height={}, Lifetime={}".format(height_fit, lifetime_fit);
    out = CalculateChiSquared(fitted_func, ws, Output="ExpProfile", weighted=True)


    # print left and right errors of parameters
    # you should not that they differ from the quadratic errors

    error_table = mtd["ExpProfile_errors"]
    lerror_height = error_table.column(3)[0]
    rerror_height= error_table.column(4)[0]
    qerror_height = error_table.column(5)[0]

    print("Error bounds of Height are {} and {}, with quadratic estimate {}".format(lerror_height, rerror_height, qerror_height))

    lerror_lifetime = error_table.column(3)[1]
    rerror_lifetime= error_table.column(4)[1]
    qerror_lifetime = error_table.column(5)[1]
    print("Error bounds of Lifetime are {} and {}, with quadratic estimate {}".format(lerror_lifetime, rerror_lifetime, qerror_lifetime))


For each problem, the error table has the following columns:

===============    ===========
Column             Description
===============    ===========
Parameter          Parameter name
Value              Parameter value passed with the Function property
Value at Min       The minimum point of the 1d slice of the :math:`\chi^{2}`. If the Function is at the minimum then
                   Value at Min should be equal to Value.
Left Error         The negative deviation from the minimum point equivalent to :math:`1\sigma`. Estimated from analisys
                   of the surface.
Right Error        The positive deviation from the minimum point equivalent to :math:`1\sigma`. Estimated from analisys
                   of the surface.
Quadratic Error    :math:`1\sigma` standard deviation in the quadratic approximation of the surface.
Chi2 Min           The value of :math:`\chi^{2}` at the minimum relative to the test point.
===============    ===========

This algorithm also reports a un-normalised probability density function (PDF) for each parameter, stored in the '<Output>_pdf' table.
This PDF is found from Eq. (1), which relates the increase in chi squared, to the probability of the parameter variation.
The pdf table contains slices of the :math:`\chi^{2}` along each parameter. It has 3 column per parameter. The first column of the 3
is the parameter values, the second has the :math:`\chi^{2}` and the third is the probability density function normalised to
have 1 at the maximum.

Usage
-----
**Example 1**

.. testcode:: CalculateChiSquaredExample

    import numpy as np

    # Create a data set
    x = np.linspace(0,1,10)
    y = 1.0 + 2.0 * x
    e = np.sqrt(y)
    ws = CreateWorkspace(DataX=x, DataY=y, DataE=e)

    # Define a function
    func = 'name=LinearBackground,A0=1.1,A1=1.9'

    # Calculate the chi squared
    chi2,chi2dof,chi2ndata,chi2W,chi2Wdof,chi2Wndata = CalculateChiSquared(func,ws)

    print('Chi squared is {:.13f}'.format(chi2))
    print('Chi squared / DOF is {:.14f}'.format(chi2dof))
    print('Chi squared / NDATA is {:.14f}'.format(chi2ndata))
    print('Chi squared weighted is {:.11f}'.format(chi2W))
    print('Chi squared weighted / DOF is {:.14f}'.format(chi2Wdof))
    print('Chi squared weighted / NDATA is {:.12f}'.format(chi2Wndata))

    # Define a function that models the data exactly
    func = 'name=LinearBackground,A0=1.0,A1=2.0'

    # Calculate the chi squared
    chi2,chi2dof,chi2ndata,chi2W,chi2Wdof,chi2Wndata = CalculateChiSquared(func,ws)

    print('Chi squared is {:.1f}'.format(chi2))
    print('Chi squared / DOF is {:.1f}'.format(chi2dof))
    print('Chi squared / NDATA is {:.1f}'.format(chi2ndata))
    print('Chi squared weighted is {:.1f}'.format(chi2W))
    print('Chi squared weighted / DOF is {:.1f}'.format(chi2Wdof))
    print('Chi squared weighted / NDATA is {:.1f}'.format(chi2Wndata))

Output:

.. testoutput:: CalculateChiSquaredExample

    Chi squared is 0.0351851851852
    Chi squared / DOF is 0.00439814814815
    Chi squared / NDATA is 0.00351851851852
    Chi squared weighted is 0.02660287840
    Chi squared weighted / DOF is 0.00332535979971
    Chi squared weighted / NDATA is 0.002660287840
    Chi squared is 0.0
    Chi squared / DOF is 0.0
    Chi squared / NDATA is 0.0
    Chi squared weighted is 0.0
    Chi squared weighted / DOF is 0.0
    Chi squared weighted / NDATA is 0.0

**Example 2**

.. testcode::

    import numpy as np
    # Create a workspace and fill it with some gaussian data and some noise
    n = 100
    x = np.linspace(-10,10,n)
    y = np.exp(-x*x/2) + np.random.normal(0.0, 0.01, n)
    e = [1] * n
    ws = CreateWorkspace(x,y,e)

    # Gefine a Gaussian with exactly the same parameters that were used to
    # generate the data
    fun_t = 'name=Gaussian,Height=%s,PeakCentre=%s,Sigma=%s'
    fun = fun_t % (1, 0, 1)
    # Test the chi squared.
    CalculateChiSquared(fun,ws,Output='Test0')
    # Check the Test0_errors table and see that the parameters are not at minimum

    # Fit the function
    res = Fit(fun,ws,Output='out')
    # res[3] is a table with the fitted parameters
    nParams = res[3].rowCount() - 1
    params = [res[3].cell(i,1) for i in range(nParams)]
    # Build a new function and populate it with the fitted parameters
    fun = fun_t % tuple(params)
    # Test the chi squared.
    CalculateChiSquared(fun,ws,Output='Test1')
    # Check the Test1_errors table and see that the parameters are at minimum now


References
----------

[1] William H. Press, Saul A. Teukolsky, William T. Vetterling, and Brian P. Flannery. 1992.
Numerical recipes in C (2nd ed.): the art of scientific computing. Cambridge University Press, USA.

[2] G. Cowan, Statistical Data Analysis, Clarendon, Oxford, 1998

.. categories::

.. sourcelink::
